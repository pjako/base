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

#include "sample_sdfRect.hlsl.h"

typedef struct FrameContext {
    Arena* perThreadTempArenas;
    f64 frameTime;
    u64 frameIdx;
} FrameContext;


#define UI_VERTEX_BUFFER_SIZE 4000
#define UI_INDEX_BUFFER_SIZE 1200

typedef struct g_State {
    Arena* arena;
    Arena* tmpArena;
    app_window window;
    rx_renderPass uiPass;
    rx_renderPipeline uiRenderPipeline;

    u32 uiIndexBufferSize;
    u32 uiVertexBufferSize;

    rx_buffer uiVertexBuffer;
    rx_buffer uiIndexBuffer;

    rx_texture atlasTexture;
    rx_sampler atlasSampler;
    rx_resGroup resourceGroup1;

    Arena* uniformCpuArena;
    rx_buffer uniformBuffer;
    rx_bumpAllocator uniformGpuArena;

    struct {
        rx_buffer buffer;
        Arena* cpuArena;
        rx_bumpAllocator gpuArena;
        rx_resGroup resGroup;
    } streamingUniforms;
#if 0
    rx_buffer vertexBuffer;
    rx_buffer indexBuffer;
    rx_sampler sampler;
    rx_texture texture;
    rx_resGroupLayout resGroupLayout;
    rx_renderPipeline renderPipeline;
    rx_resGroup resGroup;
#endif
} g_State;

void g_init(void) {
    g_State* state = (g_State*) app_getUserData();

    BaseMemory baseMem = os_getBaseMemory();

    state->tmpArena = mem_makeArena(&baseMem, MEGABYTE(10));

    state->arena = mem_makeArena(&baseMem, MEGABYTE(10));

    state->window = app_makeWindow(&(app_WindowDesc) {
        .title  = str8("UI test"),
        .width  = 375,
        .height = 668
    }); // 585 // 270
    app_showWindow(state->window);
    void* gfxHandle = app_getGraphicsHandle(state->window);

    rx_setup(&(rx_SetupDesc) {
        .context.gl.appleCaOpenGlLayer = app_getGraphicsHandle(state->window),
        .sampleCount = 1
    });


    // state->uiPass = rx_makeRenderPass(&(rx_RenderPassDesc) {
    //     .name = str8("UIPass"),
    //     .colorTargets = {{
    //         .format = rx_textureFormat_swapChain
    //     }},
    // });

    //RenderProgram* renderProgram = renderPrograms[renderProgram_UIProgram];
    rx_RenderShaderDesc uiRederShaderDesc = UIProgramShaderDesc(rx_queryBackend());
    rx_renderShader uiRenderProgram = rx_makeRenderShader(&uiRederShaderDesc);

    //rx_Shader psShader = rx_makeRenderShader(&(rx_ShaderDesc) {
    //    .code       = renderProgram->ps->code.content,
    //    .codeSize   = renderProgram->ps->code.size,
    //});

    // rx_RenderPipelineDesc renderPipelineDesc;
    // fillRenderPipelineDesc(renderProgram, &renderPipelineDesc);
    // renderPipelineDesc.colorTargets[0].blend = (rx_TargetBlendState) {
    //     .enabled = true
    // };

    state->uiRenderPipeline = rx_makeRenderPipeline(&(rx_RenderPipelineDesc) {
        .label = s8("SamplerShader"),
        .program.shader = uiRenderProgram,
        // .rasterizer.faceWinding = rx_faceWinding_counterClockwise,
        .rasterizer.cullMode = rx_cullMode_back,
        .primitiveTopology = rx_primitiveTopology_triangleList,
        .colorTargets[0].blend.enabled = true,
        .indexFormat = rx_indexType_u32,
        .layout.attrs = {
            { .format = rx_vertexFormat_f32x2 }, //float2 dstTopLeft :   vtl;
            { .format = rx_vertexFormat_f32x2 }, //float2 dstBottomRight : vbr;
            // { .format = rx_vertexFormat_f32x2 }, //float2 uvTopLeft : uvTopLeft;
            // { .format = rx_vertexFormat_f32x2 }, //float2 uvBottomRight : uvBottomRight;
            // { .format = rx_vertexFormat_f32x4 }, //float4 color : color;
            // { .format = rx_vertexFormat_f32x1 }, //float borderThickness : borderThickness; // also borderThickness
            // { .format = rx_vertexFormat_f32x1 }, //float edgeSoftness : edgeSoftness; // or shadow softness
            // { .format = rx_vertexFormat_f32x1 }, //float cornerRadius : cornerRadius;
            // { .format = rx_vertexFormat_f32x1 }, //float fontIntensity : fontIntensity;
            // { .format = rx_vertexFormat_f32x4 }, //float4 outlineColor : outlineColor;
            // { .format = rx_vertexFormat_f32x4 }, //float4 shadowColor : shadowColor;
            // { .format = rx_vertexFormat_f32x2 }, //float2 shadowOffset : shadowOffset;
        }
    });

    state->uiIndexBufferSize = UI_INDEX_BUFFER_SIZE;
    state->uiVertexBufferSize = UI_VERTEX_BUFFER_SIZE;

    // create buffers

    {
        state->uiVertexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
            .label   = str8("VertexBuffer"),
            .size    = state->uiVertexBufferSize * sizeOf(UIProgramVertex),
            .usage   = rx_bufferUsage_vertex,
        });

        state->uiIndexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
            .label   = str8("IndexBuffer"),
            .size    = state->uiIndexBufferSize * sizeOf(u32),
            .usage   = rx_bufferUsage_index,
        });
    }
    
    // top left
    Vec2 vtl = v2_make(100, 20);
    // bottom right
    Vec2 vbr = v2_make(20, 100);
    Vec4 color = v4_make(1, 0.2, 0.7, 1);
    Vec2 uvTL = v2_make(-4, -4);
    f32 fontIntensity = 1.0f;
    f32 cornerRadius = 8;
    f32 borderThickness = 0.0;
    Vec4 outlineColor = v4_make(0.2, 0.2, 0.2, 1);
    UIProgramVertex uiBlock[4] = {
        {
            .vtl = vtl,
            .vbr = vbr,
            //.uvTopLeft = uvTL,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
        {
            .vtl = vtl,
            .vbr = vbr,
            //.uvTopLeft = uvTL,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
        {
            .vtl = vtl,
            .vbr = vbr,
            //.uvTopLeft = uvTL,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
        {
            .vtl = vtl,
            .vbr = vbr,
            //.uvTopLeft = uvTL,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
    };

    uint32_t indices[] = {
        2, 1, 0,  2, 0, 3,
    };

    //uint32_t indices[] = {
    //    0, 1, 2,  3, 2, 1
    //};

    //uint32_t indices[] = {
    //    0, 1, 2,  3, 2, 1,
    //};

    rx_updateBuffer(state->uiVertexBuffer, 0, (rx_Range) {
        .content = (void*) &uiBlock[0],
        .size = sizeOf(uiBlock)
    });

    rx_updateBuffer(state->uiIndexBuffer, 0, (rx_Range) {
        .content = (void*) &indices[0],
        .size = sizeOf(indices)
    });

    state->atlasSampler = rx_makeSampler(&(rx_SamplerDesc) {
        .mipmapMode = rx_mipmapMode_linear,
        .minFilter = rx_filterMode_linear,
        .magFilter = rx_filterMode_linear,
        .compare = rx_compareFunc_equal,
    });

    rx_resGroupLayout  resourceGroupLayout1 = rx_makeResGroupLayout(&(rx_ResGroupLayoutDesc) {
        .resources[0] = {
            .type = rx_resType_texture2d
        },
        .resources[1] = {
            .type = rx_resType_sampler
        }
    });

    // create texture
    
    //ui_ImageRenderDetails* atlasTex = array_pushPtr(&state->textures); // &state->textures.elements[state->textures.count++];

    state->atlasTexture = rx_makeTexture(&(rx_TextureDesc) {
        .format = rx_textureFormat_rgba8unorm,
        .size = {
            .width  = 4,
            .height = 4
        }
    });

    {
        u32 streamingUniformSize = MEGABYTE(10);
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

    u32 uniformSize = MEGABYTE(2);

    state->uniformCpuArena = mem_makeArena(&baseMem, uniformSize + KILOBYTE(1));
    state->uniformBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_uniform,
        .size = uniformSize
    });

    state->uniformGpuArena = rx_makeBumpAllocator(&(rx_BumpAllocatorDesc) {
        .arena = state->uniformCpuArena,
        .gpuBuffer = state->uniformBuffer
    });


    state->resourceGroup1 = rx_makeResGroup(&(rx_ResGroupDesc) {
        .layout = resourceGroupLayout1,
        .usage  = rx_resGroupUsage_dynamic,
        .initalContent = {
            .storeArena = state->uniformGpuArena,
            .resources[0] = {
                .texture = state->atlasTexture
            },
            .resources[1] = {
                .sampler = state->atlasSampler
            },
        }
    });


#if 0
    // setup ui

    state->uiArena = mem_makeArena(&baseMem, MEGABYTE(10));

    for (u32 idx = 0; idx < countOf(state->uiFrameArenas); idx++) {
        state->uiFrameArenas[idx] = mem_makeArena(&baseMem, MEGABYTE(4));
    }

    S8 robotoRegularFile = os_fileRead(state->uiArena, s8(PROJECT_ROOT "/assets/fonts/Roboto-Regular.ttf"));
    // setup ui
    ui_setup(&(ui_SetupDesc) {
        .arena = state->uiArena,
        .fonts[0] = {
            .fontFile = robotoRegularFile,
            .name = str8("Roboto"),
        },
        .atlasTextureRef = array_atIndex(&state->textures, 0)
    });

    {
        rx_texture magePortrait = tcg_loadAndSetupImage(s8(PROJECT_ROOT "/assets/images/magus_128.png"), state->uiArena);
        i32 mageIdx = state->textures.count;
        ui_ImageRenderDetails* nelfTex = array_pushPtr(&state->textures);
        nelfTex->texture = magePortrait;

        nelfTex->resGroup = rx_makeResGroup(&(rx_ResGroupDesc) {
            .layout = resourceGroupLayout1,
            .usage  = rx_resGroupUsage_dynamic,
            .initalContent = {
                .storeArena = state->uniformGpuArena,
                .resources[0] = {
                    .texture = nelfTex->texture
                },
                .resources[1] = {
                    .sampler = state->atlasSampler
                }
            }
        });
    }

    {
        rx_texture battleMap = tcg_loadAndSetupImage(s8(PROJECT_ROOT "/assets/images/battlemap_0-0.png"), state->uiArena);
        i32 battleMapIdx = state->textures.count;
        state->testBattleMapTextureIdx = battleMapIdx;
        ui_ImageRenderDetails* battleMapDetails = array_pushPtr(&state->textures);
        battleMapDetails->texture = battleMap;

        battleMapDetails->resGroup = rx_makeResGroup(&(rx_ResGroupDesc) {
            .layout = resourceGroupLayout1,
            .usage  = rx_resGroupUsage_dynamic,
            .initalContent = {
                .storeArena = state->uniformGpuArena,
                .resources[0] = {
                    .texture = battleMapDetails->texture
                },
                .resources[1] = {
                    .sampler = state->atlasSampler
                }
            }
        });
        state->textures.count += 1;
    }
#endif

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

    UIDynResDetails dynDetails = {0};
    //dynDetails.fontTextureWidth = 1024;
    //dynDetails.fontSdfPixelRange = 5;
    dynDetails.screenSize.x = windowFrameBufferSize.x;
    dynDetails.screenSize.y = windowFrameBufferSize.y;

    Vec2 screenCenter = v2_make(0.5f * windowFrameBufferSize.x, 0.5f * windowFrameBufferSize.y);

    // top left
    Vec2 vtl = v2_make(screenCenter.x - 120.0f, screenCenter.y - 120.0f);
    // bottom right
    Vec2 vbr = v2_make(screenCenter.x + 120.0f, screenCenter.y + 120.0f);
    Vec4 color = v4_make(1, 0.2, 0.7, 1);
    Vec2 uvTL = v2_make(-4, -4);
    f32 fontIntensity = 1.0f;
    f32 cornerRadius = 8;
    f32 borderThickness = 0.0;
    Vec4 outlineColor = v4_make(0.2, 0.2, 0.2, 1);
    UIProgramVertex uiBlock[4] = {
        {
            .vtl = vtl,
            .vbr = vbr,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
        {
            .vtl = vtl,
            .vbr = vbr,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
        {
            .vtl = vtl,
            .vbr = vbr,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
        {
            .vtl = vtl,
            .vbr = vbr,
            //.color = color,
            //.borderThickness = borderThickness,
            //.cornerRadius = cornerRadius,
            //.outlineColor = outlineColor
        },
    };

    rx_updateBuffer(state->uiVertexBuffer, 0, (rx_Range) {
        .content = (void*) &uiBlock[0],
        .size = sizeOf(uiBlock)
    });

    rx_updateBuffer(state->streamingUniforms.buffer, 0, (rx_Range) {
        .content = &dynDetails,
        .size = sizeOf(dynDetails)
    });

    u64 dynamicResGroup0 = rx_bumpAllocatorPushData(state->streamingUniforms.gpuArena, (rx_Range) {
        .content = &dynDetails,
        .size = sizeOf(dynDetails)
    });

    mem_scoped(tmpMem, state->arena) {
        rx_RenderCmdBuilder cmdBuilder;
        rx_renderCmdBuilderInit(tmpMem.arena, &cmdBuilder, 100);
        rx_renderCmdBuilderSetPipeline(&cmdBuilder, state->uiRenderPipeline);
        rx_renderCmdBuilderSetVertexBuffer0(&cmdBuilder, state->uiVertexBuffer);
        rx_renderCmdBuilderSetIndexBuffer(&cmdBuilder, state->uiIndexBuffer);
        rx_renderCmdBuilderSetDynResGroup0(&cmdBuilder, dynamicResGroup0);
        rx_renderCmdBuilderSetResGroup1(&cmdBuilder, state->resourceGroup1);

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
}

void g_cleanup(void) {
    g_State* state = (g_State*) app_getUserData();
    rx_shutdown();

    mem_destroyArena(state->arena);
    app_setUserData(NULL);
}

i32 app_main(i32 argCount, char* args[]) {

    app_initApplication(&(app_ApplicationDesc) {
        .init    = g_init,
        .update  = g_update,
        .cleanup = g_cleanup,
        .event   = g_event
    });

    BaseMemory baseMem = os_getBaseMemory();
    Arena* arena = mem_makeArena(&baseMem, MEGABYTE(20));
    g_State* state = mem_arenaPushStructZero(arena, g_State);
    state->arena = arena;
    app_setUserData(state);

    return 0;
}