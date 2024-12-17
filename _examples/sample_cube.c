#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_math.h"
#include "base/base_str.h"
#include "base/base_time.h"
#include "base/base_color.h"
#include "os/os.h"
#include "log/log.h"
#include "app/app.h"
#include "rx/rx.h"
#include "rx/rx_helper.h"

#include "sample_cube.hlsl.h"

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
    rx_swapChain swapChain;
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
    u64 frameCount;
    f64 lastFrameTime;
} g_State;

void g_init(void) {
    BaseMemory baseMem = os_getBaseMemory();
    Arena* mainArena = mem_makeArena(&baseMem, MEGABYTE(20));
    g_State* state = mem_arenaPushStructZero(mainArena, g_State);
    app_setUserData(state);
    state->arena = mainArena;

    state->window = app_makeWindow(&(app_WindowDesc) {
        .title  = s8("Cube Sample"),
        .width  = 600,
        .height = 400
    });

    //app_showWindow(state->window);

    rx_setup(&(rx_SetupDesc) {
        .context.gl.appleCaOpenGlLayer = app_getGraphicsHandle(state->window),
        .sampleCount = 1
    });

    state->swapChain = rx_makeSwapChain(&(rx_SwapChainDesc) {
        .windows.hWnd = app_getWin32WindowHandle(state->window)
    });

    // uniform buffer

    {
        u32 streamingUniformSize = MEGABYTE(1);
        
        rx_buffer buffer = rx_makeBuffer(&(rx_BufferDesc) {
            .usage = rx_bufferUsage_uniform,
            .size = streamingUniformSize
        });
        state->streamingUniforms.buffer = buffer;
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

    // cube vertex buffer

    Rgba cubeColorFront = rgba_pink;
    Rgba cubeColorBack = rgba_black;
    Rgba cubeColorLeft = rgba_green;
    Rgba cubeColorRight = rgba_yellow;
    Rgba cubeColorBottom = rgba_red;
    Rgba cubeColorTop = rgba_blue;
    
    struct {Vec3 pos; Rgba color;} vertices[] = {
        // front
        { .pos = vec3_make(-1.0, -1.0, -1.0), .color = cubeColorFront },
        { .pos = vec3_make( 1.0, -1.0, -1.0), .color = cubeColorFront },
        { .pos = vec3_make( 1.0,  1.0, -1.0), .color = cubeColorFront },
        { .pos = vec3_make(-1.0,  1.0, -1.0), .color = cubeColorFront },
        // back
        { .pos = vec3_make(-1.0, -1.0,  1.0), .color = cubeColorBack },
        { .pos = vec3_make( 1.0, -1.0,  1.0), .color = cubeColorBack },
        { .pos = vec3_make( 1.0,  1.0,  1.0), .color = cubeColorBack },
        { .pos = vec3_make(-1.0,  1.0,  1.0), .color = cubeColorBack },
        // left
        { .pos = vec3_make(-1.0, -1.0, -1.0), .color = cubeColorLeft },
        { .pos = vec3_make(-1.0,  1.0, -1.0), .color = cubeColorLeft },
        { .pos = vec3_make(-1.0,  1.0,  1.0), .color = cubeColorLeft },
        { .pos = vec3_make(-1.0, -1.0,  1.0), .color = cubeColorLeft },
        // right
        { .pos = vec3_make(1.0, -1.0,  -1.0), .color = cubeColorRight },
        { .pos = vec3_make(1.0,  1.0,  -1.0), .color = cubeColorRight },
        { .pos = vec3_make(1.0,  1.0,   1.0), .color = cubeColorRight },
        { .pos = vec3_make(1.0, -1.0,   1.0), .color = cubeColorRight },
        // bottom
        { .pos = vec3_make(-1.0, -1.0, -1.0), .color = cubeColorBottom },
        { .pos = vec3_make(-1.0, -1.0,  1.0), .color = cubeColorBottom },
        { .pos = vec3_make( 1.0, -1.0,  1.0), .color = cubeColorBottom },
        { .pos = vec3_make( 1.0, -1.0, -1.0), .color = cubeColorBottom },
        // top
        { .pos = vec3_make(-1.0,  1.0, -1.0), .color = cubeColorTop },
        { .pos = vec3_make(-1.0,  1.0,  1.0), .color = cubeColorTop },
        { .pos = vec3_make( 1.0,  1.0,  1.0), .color = cubeColorTop },
        { .pos = vec3_make( 1.0,  1.0, -1.0), .color = cubeColorTop }
    };

    state->vertexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(vertices)
    });

    rx_updateBuffer(state->vertexBuffer, 0, (rx_Range) {
        .size = sizeOf(vertices),
        .content = (void*) vertices
    });


    // create an index buffer for the cube
    uint32_t indices[] = {
        0, 1, 2,  0, 2, 3,
        6, 5, 4,  7, 6, 4,
        8, 9, 10,  8, 10, 11,
        14, 13, 12,  15, 14, 12,
        16, 17, 18,  16, 18, 19,
        22, 21, 20,  23, 22, 20
    };

    state->indexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(indices)
    });

    rx_updateBuffer(state->indexBuffer, 0, (rx_Range) {
        .size = sizeOf(indices),
        .content = (void*) indices
    });

    // sampler state

    state->sampler = rx_makeSampler(&(rx_SamplerDesc) {
        .mipmapMode = rx_mipmapMode_nearest,
        .compare = rx_compareFunc_never,
    });

    // shader


    rx_RenderShaderDesc sampleCubeShaderDesc = CubeProgramShaderDesc(rx_queryBackend());
    rx_renderShader sampleCubeShader = rx_makeRenderShader(&sampleCubeShaderDesc);

    // ResGroupLayout
    state->resGroupLayout = rx_makeResGroupLayout(&(rx_ResGroupLayoutDesc) {
        .uniformSize = sizeOf(Mat4)
    });

    // pipeline

    state->renderPipeline = rx_makeRenderPipeline(&(rx_RenderPipelineDesc) {
        .label = s8("SamplerShader"),
        .program.shader = sampleCubeShader,
        .rasterizer.cullMode = rx_cullMode_back,
        .primitiveTopology = rx_primitiveTopology_triangleList,
        .indexFormat = rx_indexType_u32,
        .layout = {
            // pos
            .attrs[0] = {
                .format = rx_vertexFormat_f32x3
            },
            // color
            .attrs[1] = {
                .format = rx_vertexFormat_f32x4
            }
        },
        .depthStencil = {
            .depthCompareFunc = rx_compareFunc_lessEqual,
            .depthWriteEnabled = true,
        },
    });

    // show window!

    //app_showWindow(state->window);
}

void g_event(app_AppEvent* event) {
    g_State* state = (g_State*) app_getUserData();

}


void g_update(void) {
    g_State* state = (g_State*) app_getUserData();
    app_showWindow(state->window);

    
    u64 timeCount = tm_currentCount();
    if (state->lastFrameTimeCount == 0) {
        state->lastFrameTimeCount = timeCount;
    }
    u64 timeCountDiffSinceLastFrame = (timeCount - state->lastFrameTimeCount);
    state->lastFrameTimeCount = timeCount;
    // something is wrong with the performance timer on windows
    tm_FrequencyInfo frequencyInfo = tm_getPerformanceFrequency();
    u64 frameTime = tm_countToNanoseconds(frequencyInfo, timeCountDiffSinceLastFrame);
    u64 roundedFrameTime = tm_roundToCommonRefreshRate(frameTime);

    state->lastFrameTime += (f64) tm_countToSeconds(roundedFrameTime);
    f32 frameTimeInSeconds = (f32) state->lastFrameTime;
    // static f32 frameC;
    // frameTimeInSeconds = frameC + 0.01f;
    // frameC = frameTimeInSeconds;
    //log_debugFmt(state->arena, s8("timeCount: {} size: ({}|{})"), timeCountDiffSinceLastFrame, windowFrameBufferSize.x, windowFrameBufferSize.y);

    if (frameTimeInSeconds == 0) {
        return;
    }

    Vec2 windowSize = app_getWindowSizeF32(state->window);
    Vec2 windowFrameBufferSize = app_getWindowSizeF32(state->window);//app_getWindowFrameBufferSizeF32(state->window);

    if (windowSize.x <= 0 || windowSize.y <= 0) {
        return;
    }

    log_debugFmt(state->arena, s8("size: ({}|{})"), windowSize.x, windowSize.y);
    
    rx_texture currentSwapTexture = rx_getCurrentSwapTexture(state->swapChain);

    rx_renderPass renderPass = rx_makeRenderPass(&(rx_RenderPassDesc) {
        .colorTargets[0].target = currentSwapTexture,
        .colorTargets[0].clearColor = rgba_chocolate,
        .depthStencil.active = true,
        // Only needed on legacy apis (OpenGL/WebGL) for the "default pass"
        .width = windowFrameBufferSize.x,
        .height = windowFrameBufferSize.y
    }, NULL);


    // const float t = (frameTimeInSeconds * 60.0f);
    Mat4 proj = mat4_perspective(60.0f, windowFrameBufferSize.x / windowFrameBufferSize.y, 0.01f, 10.0f);
    Mat4 view = mat4_lookAt(vec3_make(0.0f, 1.5f, 6.0f), vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.0f, 1.0f, 0.0f));
    Mat4 view_proj = mat4_multiply(proj, view);
    //state->rx += 1.0f * t; state->ry += 2.0f * t;
    Mat4 rxm = mat4_rotate(frameTimeInSeconds * 35.0f, vec3_make(1.0f, 0.0f, 0.0f));
    Mat4 rym = mat4_rotate(frameTimeInSeconds * 35.0f, vec3_make(0.0f, 1.0f, 0.0f));
    Mat4 rzm = mat4_rotate(frameTimeInSeconds * 35.0f, vec3_make(0.0f, 1.0f, 0.0f));

    Mat4 model = mat4_multiply(mat4_multiply(rxm, rym), rzm);
    Mat4 mvp = mat4_multiply(view_proj, model);


    mem_scoped(tmpMem, state->arena) {
        rx_RenderCmdBuilder cmdBuilder;
        rx_renderCmdBuilderInit(tmpMem.arena, &cmdBuilder, 100);

        #if 0
        shd_SampleInstancingProgramCmdBuilder instancingProgBuilder = shd_sampleInstancingProgramCmdBuilder(rx_queryBackend(), cmdBuilder, state->renderPipeline);

        shd_sampleInstanceProgramCmd_builderSetVertexBuffer0(&instancingProgBuilder, state->vertexBuffer);
        shd_sampleInstanceProgramCmd_builderSetIndexBuffer(&instancingProgBuilder, state->indexBuffer);
        shd_sampleInstanceProgramCmd_makeSetDynGroup0(&instancingProgBuilder, &resGroup1, state->streamingUniforms.gpuArena);
        shd_sampleInstanceProgramCmd_draw(&instancingProgBuilder, 0, 6, 0, 0);
        #endif

        /**/
        rx_renderCmdBuilderSetPipeline(&cmdBuilder, state->renderPipeline);

        rx_renderCmdBuilderSetVertexBuffer0(&cmdBuilder, state->vertexBuffer);
        rx_renderCmdBuilderSetIndexBuffer(&cmdBuilder, state->indexBuffer);

        // check if uploading works
        u64 offset = rx_bumpAllocatorPushData(state->streamingUniforms.gpuArena, (rx_Range) {
            .content = (void*) &mvp,
            .size = sizeOf(mvp)
        });
        rx_renderCmdBuilderSetDynResGroup0(&cmdBuilder, offset);
        rx_renderCmdBuilderDraw(&cmdBuilder, 0, 36, 0, 0);

        rx_DrawArea areas = {
            .resGroupDynamicOffsetBuffers = state->streamingUniforms.resGroup,
            .drawCount = 1
        };

        // set cmds to render pass
        rx_setRenderPassDrawList(renderPass, &areas, 1, cmdBuilder.drawList);
        
        // finish the frame
        rx_commit();
    }
    state->frameCount++;
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