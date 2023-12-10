#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_math.h"
#include "base/base_str.h"
#include "base/base_color.h"
#include "os/os.h"
#include "app/app.h"
#include "rx/rx.h"
#include "rx/rx_helper.h"

#include "sample_texture.hlsl.h"

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
} g_State;

void g_init(void) {
    BaseMemory baseMem = os_getBaseMemory();
    Arena* mainArena = mem_makeArena(&baseMem, MEGABYTE(1));
    g_State* state = mem_arenaPushStruct(mainArena, g_State);
    app_setUserData(state);
    state->arena = mainArena;

    state->window = app_makeWindow(&(app_WindowDesc) {
        .title  = s8("Sample texture"),
        .width  = 375,
        .height = 668
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

    f32 vertexData[] = {
        // positions            // colors
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f,   /* Vertex 0 */
        -0.5f,  0.5f, 0.0f,     0.0f, 1.0f,   /* Vertex 1 */
        0.5f,  0.5f, 0.0f,      1.0f, 1.0f,   /* Vertex 2 */
        0.5f, -0.5f, 0.0f,      1.0f, 0.0f    /* Vertex 3 */
    };

    state->vertexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(vertexData)
    });

    rx_updateBuffer(state->vertexBuffer, 0, (rx_Range) {
        .size = sizeOf(vertexData),
        .content = vertexData
    });


    static uint32_t indexData[] = {
        0, 1, 2, 2, 3, 0
    };

    state->indexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(indexData)
    });

    rx_updateBuffer(state->indexBuffer, 0, (rx_Range) {
        .size = sizeOf(indexData),
        .content = indexData
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


    rx_RenderShaderDesc texShaderDesc = SampleTextureProgramShaderDesc(rx_queryBackend());
    rx_renderShader sampleShader = rx_makeRenderShader(&texShaderDesc);

    // ResGroupLayout
    state->resGroupLayout = rx_makeResGroupLayout(&(rx_ResGroupLayoutDesc) {
        .resources[0] = {
            .type = rx_resType_texture2d
        },
        .resources[1] = {
            .type = rx_resType_sampler
        }
    });

    // pipeline

    state->renderPipeline = rx_makeRenderPipeline(&(rx_RenderPipelineDesc) {
        .label = s8("SamplerShader"),
        .program.shader = sampleShader,
        .rasterizer.cullMode = rx_cullMode_back,
        .primitiveTopology = rx_primitiveTopology_triangleList,
        .indexFormat = rx_indexType_u32,
        // pos
        .layout.attrs[0] = {
            .format = rx_vertexFormat_f32x3,
        },
        // uv
        .layout.attrs[1] = {
            .format = rx_vertexFormat_f32x2,
        },
    });

    // dynamic state

    f32 offset = 0;
    state->resGroup = rx_makeResGroup(&(rx_ResGroupDesc) {
        .layout = state->resGroupLayout,
        .usage  = rx_resGroupUsage_dynamic,
        .initalContent = {
            .storeArena = state->uniformGpuArena,
            .resources[0] = {
                .texture = state->texture
            },
            .resources[1] = {
                .sampler = state->sampler
            },
        }
    });

    // show window!

    app_showWindow(state->window);
}

void g_event(app_AppEvent* event) {
    g_State* state = (g_State*) app_getUserData();

}


void g_update(void) {
    g_State* state = (g_State*) app_getUserData();
    Vec2 windowFrameBufferSize = app_getWindowFrameBufferSizeF32(state->window);
    
    rx_texture texture = rx_getCurrentSwapTexture();


    rx_renderPass renderPass = rx_makeRenderPass(&(rx_RenderPassDesc) {
        .colorTargets[0].target = texture,
        .colorTargets[0].clearColor = rgba_chocolate,
        // Only needed on legacy apis (OpenGL/WebGL) for the "default pass"
        .width = windowFrameBufferSize.x,
        .height = windowFrameBufferSize.y
    }, NULL);


    static f32 offsets[2];
    static f32 dir = 1;

    mem_scoped(tmpMem, state->arena) {
        rx_RenderCmdBuilder cmdBuilder;
        rx_renderCmdBuilderInit(tmpMem.arena, &cmdBuilder, 100);
        rx_renderCmdBuilderSetPipeline(&cmdBuilder, state->renderPipeline);
        rx_renderCmdBuilderSetVertexBuffer0(&cmdBuilder, state->vertexBuffer);
        rx_renderCmdBuilderSetIndexBuffer(&cmdBuilder, state->indexBuffer);
        rx_renderCmdBuilderSetResGroup1(&cmdBuilder, state->resGroup);
        // check if uploading works
        u64 offset = rx_bumpAllocatorPushData(state->streamingUniforms.gpuArena, (rx_Range) {
            .content = &offsets[0],
            .size = sizeOf(offsets)
        });
        rx_renderCmdBuilderSetDynResGroup0(&cmdBuilder, offset);
        rx_renderCmdBuilderDraw(&cmdBuilder, 0, 6, 0, 0);

        rx_DrawArea areas = {
            .resGroupDynamicOffsetBuffers = state->streamingUniforms.resGroup,
            .drawCount = 1
        };

        // set cmds to render pass
        rx_setRenderPassDrawList(renderPass, &areas, 1, cmdBuilder.drawList);
        
        // finish the frame
        rx_commit();
    }

    offsets[0] += 0.015f * dir;
    if (f32_abs(offsets[0]) >= 1.0f) {
        dir *= -1.0f;
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