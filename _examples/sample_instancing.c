#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_math.h"
#include "base/base_str.h"
#include "base/base_time.h"
#include "base/base_color.h"
#include "os/os.h"
#include "app/app.h"
#include "log/log.h"
#include "rx/rx.h"
#include "rx/rx_helper.h"

#include "sample_instancing.hlsl.h"

#define MAX_PARTICLES (512 * 1024)
#define NUM_PARTICLES_EMITTED_PER_FRAME (10)

typedef struct FrameContext {
    Arena* perThreadTempArenas;
    f64 frameTime;
    u64 frameIdx;
} FrameContext;

typedef struct g_State {
    Arena* arena;
    app_window window;
    rx_buffer vertexBuffer;
    rx_buffer indexBuffer;
    rx_buffer perInstancePosAndVelocityVertexBuffer;
    rx_sampler sampler;
    rx_texture texture;
    rx_resGroupLayout resGroupLayout;
    rx_renderPipeline renderPipeline;
    rx_resGroup resGroup;
    Arena* uniformCpuArena;
    rx_buffer uniformBuffer;
    rx_bumpAllocator uniformGpuArena;
    struct {
        rx_buffer buffer;
        Arena* cpuArena;
        rx_bumpAllocator gpuArena;
        rx_resGroup resGroup;
    } streamingUniforms;

    u64 lastFrameTimeCount;
    f32 ry;

    u32 seed;
    u32 curNumParticles;
    Vec3 pos[MAX_PARTICLES];
    Vec3 vel[MAX_PARTICLES];
} g_State;

void g_init(void) {
    BaseMemory baseMem = os_getBaseMemory();
    Arena* mainArena = mem_makeArena(&baseMem, MEGABYTE(20));
    g_State* state = mem_arenaPushStruct(mainArena, g_State);
    app_setUserData(state);
    state->arena = mainArena;

    state->window = app_makeWindow(&(app_WindowDesc) {
        .title  = s8("Instancing Sample"),
        .width  = 600,
        .height = 400
    });

    rx_setup(&(rx_SetupDesc) {
        .context.gl.appleCaOpenGlLayer = app_getGraphicsHandle(state->window),
        .sampleCount = 1
    });

    // uniform buffer

    {
        u32 streamingUniformSize = MEGABYTE(1);
        state->streamingUniforms.buffer = rx_makeBuffer(&(rx_BufferDesc) {
            .usage = rx_bufferUsage_uniform,
            .size = streamingUniformSize
        });
        state->streamingUniforms.cpuArena = mem_makeArena(&baseMem, streamingUniformSize + KILOBYTE(1));
        state->streamingUniforms.gpuArena = rx_makeBumpAllocator(&(rx_BumpAllocatorDesc) {
            .arena = state->streamingUniforms.cpuArena,
            .gpuBuffer = state->streamingUniforms.buffer
        });
        state->streamingUniforms.resGroup = rx_makeDynamicBufferResGroup(state->streamingUniforms.buffer, state->streamingUniforms.buffer);
    }

    u32 uniformSize = MEGABYTE(6);

    state->uniformCpuArena = mem_makeArena(&baseMem, uniformSize + KILOBYTE(1));
    state->uniformBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_uniform,
        .size = uniformSize
    });

    state->uniformGpuArena = rx_makeBumpAllocator(&(rx_BumpAllocatorDesc) {
        .arena = state->uniformCpuArena,
        .gpuBuffer = state->uniformBuffer
    });

    // buffers

    // vertex buffer for static geometry, goes into vertex-buffer-slot 0
    const f32 r = 0.05f;
    const f32 vertexData[] = {
        // positions            colors
        0.0f,   -r, 0.0f,       1.0f, 0.0f, 0.0f, 1.0f,
           r, 0.0f, r,          0.0f, 1.0f, 0.0f, 1.0f,
           r, 0.0f, -r,         0.0f, 0.0f, 1.0f, 1.0f,
          -r, 0.0f, -r,         1.0f, 1.0f, 0.0f, 1.0f,
          -r, 0.0f, r,          0.0f, 1.0f, 1.0f, 1.0f,
        0.0f,    r, 0.0f,       1.0f, 0.0f, 1.0f, 1.0f
    };

    state->vertexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(vertexData)
    });

    rx_updateBuffer(state->vertexBuffer, 0, (rx_Range) {
        .size = sizeOf(vertexData),
        .content = (void*) vertexData
    });

    const u32 indexData[] = {
        0, 1, 2,    0, 2, 3,    0, 3, 4,    0, 4, 1,
        5, 1, 2,    5, 2, 3,    5, 3, 4,    5, 4, 1
    };

    state->indexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(indexData)
    });

    rx_updateBuffer(state->indexBuffer, 0, (rx_Range) {
        .size = sizeOf(indexData),
        .content = (void*) indexData
    });

    state->perInstancePosAndVelocityVertexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .label = s8("PerInstancePosAndVelocity"),
        .usage = rx_bufferUsage_vertex | rx_bufferUsage_stream,
        .size = MAX_PARTICLES * sizeof(Vec3)
    });

    // texture

    Rgba8 textureData[] = {
        rgba8_aqua, rgba8_lightGoldenRodYellow,
        rgba8_forestGreen, rgba8_darkSalmon
    };

    state->texture = rx_makeTexture(&(rx_TextureDesc) {
        .label   = "MyTexture",
        .format  = rx_textureFormat_rgba8unorm,
        .usage   = rx_textureUsage_sampled,
        .size.width = 2,
        .size.height = 2,
        .data.subimage[0][0] = {
            .content = (u8*) &textureData[0],
            .size    = sizeof(textureData)
        }
    });

    // sampler state

    state->sampler = rx_makeSampler(&(rx_SamplerDesc) {
        .mipmapMode = rx_mipmapMode_nearest,
        .compare = rx_compareFunc_never,
    });

    // shader


    rx_RenderShaderDesc texShaderDesc = SampleInstancingProgramShaderDesc(rx_queryBackend());
    rx_renderShader sampleShader = rx_makeRenderShader(&texShaderDesc);

    // ResGroupLayout
    state->resGroupLayout = rx_makeResGroupLayout(&(rx_ResGroupLayoutDesc) {
        .uniformSize = sizeOf(Mat4)
    });

    // pipeline

    state->renderPipeline = rx_makeRenderPipeline(&(rx_RenderPipelineDesc) {
        .label = s8("SamplerShader"),
        .program.shader = sampleShader,
        .rasterizer.cullMode = rx_cullMode_back,
        .primitiveTopology = rx_primitiveTopology_triangleList,
        .indexFormat = rx_indexType_u32,
        .layout = {
            .buffers[1].stepFunc = rx_vertexStep_perInstance,
            // pos
            .attrs[0] = {
                .format = rx_vertexFormat_f32x3,
                .bufferIndex = 0
            },
            // color
            .attrs[1] = {
                .format = rx_vertexFormat_f32x4,
                .bufferIndex = 0
            },
            // instancePos
            .attrs[2] = {
                .format = rx_vertexFormat_f32x3,
                .bufferIndex = 1
            },
        },
        .depthStencil = {
            .depthCompareFunc = rx_compareFunc_lessEqual,
            .depthWriteEnabled = false,
        },
    });

    // show window!

    app_showWindow(state->window);
}

void g_event(app_AppEvent* event) {
    g_State* state = (g_State*) app_getUserData();

}

#include <stdlib.h>
void g_update(void) {
    g_State* state = (g_State*) app_getUserData();

    
    u64 timeCount = tm_currentCount();
    if (state->lastFrameTimeCount == 0) {
        state->lastFrameTimeCount = timeCount;
    }
    u64 timeCountDiffSinceLastFrame = (timeCount - state->lastFrameTimeCount);
    state->lastFrameTimeCount = timeCount;
    
    tm_FrequencyInfo frequencyInfo = tm_getPerformanceFrequency();
    u64 frameTime = tm_countToNanoseconds(frequencyInfo, timeCountDiffSinceLastFrame);
    u64 roundedFrameTime = tm_roundToCommonRefreshRate(frameTime);
    f32 frameTimeInSeconds = 1.0f / 60.0f;// (f32) tm_countToSeconds(roundedFrameTime);


    Vec2 windowSize = app_getWindowSizeF32(state->window);
    Vec2 windowFrameBufferSize = app_getWindowFrameBufferSizeF32(state->window);
    
    rx_texture texture = rx_getCurrentSwapTexture();

    rx_renderPass renderPass = rx_makeRenderPass(&(rx_RenderPassDesc) {
        .colorTargets[0].target = texture,
        .colorTargets[0].clearColor = rgba_chocolate,
        // Only needed on legacy apis (OpenGL/WebGL) for the "default pass"
        .width = windowFrameBufferSize.x,
        .height = windowFrameBufferSize.y
    }, NULL);

    u32 seed = state->seed + 1;

    // emit new particles
    for (int i = 0; i < NUM_PARTICLES_EMITTED_PER_FRAME; i++) {
        if (state->curNumParticles < MAX_PARTICLES) {
            state->pos[state->curNumParticles] = vec3_zero();
            state->vel[state->curNumParticles] = vec3_make(
                //f32_random(0.0f, 1.0f, &seed) - 0.5f,
                //f32_random(0.0f, 0.5f, &seed) * 0.5f + 2.0f,
                //f32_random(0.0f, 1.0f, &seed) - 0.5f
                f32_random(0.0f, 0.5f, &seed) - 0.5f,
                f32_random(0.0f, 0.5f, &seed) * 0.5f + 2.0f,
                f32_random(0.0f, 0.5f, &seed) - 0.5f
            );

            state->curNumParticles++;
        } else {
            break;
        }
    }

    state->seed = seed;

    // update particle positions
    #if 1
    for (int i = 0; i < state->curNumParticles; i++) {
        state->vel[i].y -= 1.0f * frameTimeInSeconds;
        state->pos[i] = vec3_add(state->pos[i], vec3_multiply(state->vel[i], vec3_splat(frameTimeInSeconds)));

        // bounce back from 'ground'
        if (state->pos[i].y < -2.0f) {
            state->pos[i].y = -1.8f;
            state->vel[i].y = -state->vel[i].y;
            state->vel[i] = vec3_multiply(state->vel[i], vec3_splat(0.8f));
        }
    }
    #endif

    // update instance data
    rx_updateBuffer(state->perInstancePosAndVelocityVertexBuffer, 0, (rx_Range) {
        .content = state->pos,
        .size = (size_t)state->curNumParticles * sizeOf(state->pos[0])
    });

    // model-view-projection matrix
    Mat4 proj = mat4_perspective(60.0f, windowSize.x / windowSize.y, 0.01f, 50.0f);
    Mat4 view = mat4_lookAt(vec3_make(0.0f, 1.5f, 12.0f), vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.0f, 1.0f, 0.0f));
    Mat4 viewProj = mat4_multiply(proj, view);
    //state->ry += 60.0f * frameTimeInSeconds;

#if 0

    // model-view-projection matrix
    hmm_mat4 proj = HMM_Perspective(60.0f, sapp_widthf()/sapp_heightf(), 0.01f, 50.0f);
    hmm_mat4 view = HMM_LookAt(HMM_Vec3(0.0f, 1.5f, 12.0f), HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));
    hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);
    state.ry += 60.0f * frame_time;
    vs_params_t vs_params;
    vs_params.mvp = HMM_MultiplyMat4(view_proj, HMM_Rotate(state.ry, HMM_Vec3(0.0f, 1.0f, 0.0f)));


    shd_ResGroup1 resGroup1;
    resGroup1.mvp = HMM_MultiplyMat4(viewProj, HMM_Rotate(state->ry, HMM_Vec3(0.0f, 1.0f, 0.0f)));
#endif
    Mat4 mvp = mat4_multiply(viewProj, mat4_rotate(state->ry, vec3_make(0.0f, 1.0f, 0.0f)));
    TransformDataResGroup dynResGroup = {
        .mvp = mvp
    };
    mem_scoped(tmpMem, state->arena) {
        rx_RenderCmdBuilder cmdBuilder;
        rx_renderCmdBuilderInit(tmpMem.arena, &cmdBuilder, 100);

        #if 0
        shd_SampleInstancingProgramCmdBuilder instancingProgBuilder = shd_sampleInstancingProgramCmdBuilder(rx_queryBackend(), cmdBuilder, state->renderPipeline);

        shd_sampleInstanceProgramCmdBuilder_setPipeline(&cmdBuilder, state->renderPipeline);

        shd_sampleInstanceProgramCmdBuilder_setVertexBuffer0(&instancingProgBuilder, state->vertexBuffer);
        shd_sampleInstanceProgramCmdBuilder_builderSetIndexBuffer(&instancingProgBuilder, state->indexBuffer);
        shd_sampleInstanceProgramCmdBuilder_setDynGroup0(&instancingProgBuilder, dynGroup0);
        shd_sampleInstanceProgramCmdBuilder_draw(&instancingProgBuilder, 0, 6, 0, 0);
        #endif

        /**/
        rx_renderCmdBuilderSetPipeline(&cmdBuilder, state->renderPipeline);

        rx_renderCmdBuilderSetVertexBuffer0(&cmdBuilder, state->vertexBuffer);
        rx_renderCmdBuilderSetIndexBuffer(&cmdBuilder, state->indexBuffer);

        // per instance buffer
        rx_renderCmdBuilderSetVertexBuffer1(&cmdBuilder, state->perInstancePosAndVelocityVertexBuffer);

        // check if uploading works
        u64 offset = rx_bumpAllocatorPushData(state->streamingUniforms.gpuArena, (rx_Range) {
            .content = (void*) &dynResGroup,
            .size = sizeOf(dynResGroup)
        });
        rx_renderCmdBuilderSetDynResGroup0(&cmdBuilder, offset);
        rx_renderCmdBuilderSetInstanceCount(&cmdBuilder, state->curNumParticles);
        rx_renderCmdBuilderDraw(&cmdBuilder, 0, 24, 0, 0);

        rx_DrawArea areas = {
            .resGroupDynamicOffsetBuffers = state->streamingUniforms.resGroup,
            .drawCount = 1
        };

        // set cmds to render pass
        rx_setRenderPassDrawList(renderPass, &areas, 1, cmdBuilder.drawList);
        
        // finish the frame
        rx_commit();
    }
}

void g_cleanup(void) {
    g_State* state = (g_State*) app_getUserData();
    rx_shutdown();

    mem_destroyArena(state->arena);
    app_setUserData(NULL);
}

i32 app_main(i32 argCount, char* args[]) {

    u32 first  = alignUp(1, 256);
    u32 second = alignUp(first + 1, 256);
    u32 third  = alignUp(second + 1, 256);

    app_initApplication(&(app_ApplicationDesc) {
        .init    = g_init,
        .update  = g_update,
        .cleanup = g_cleanup,
        .event   = g_event
    });

    return 0;
}