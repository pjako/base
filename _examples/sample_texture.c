#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "base/base_color.h"
#include "os/os.h"
#include "app/app.h"
#include "rx/rx.h"
#include "rx/rx_helper.h"

//#include "triangle.hlsl.h"

typedef struct FrameContext {
    Arena* perThreadTempArenas;
    f64 frameTime;
    u64 frameIdx;
} FrameContext;

typedef struct g_State {
    Arena* arena;
    app_window window;
    rx_buffer vertexBuffer;
    //rx_buffer indexBuffer;
    rx_resGroupLayout resGroupLayout;
    rx_renderPipeline renderPipeline;
    rx_resGroup resGroup;
} g_State;

void g_init(void) {
    BaseMemory baseMem = os_getBaseMemory();
    Arena* mainArena = mem_makeArena(&baseMem, MEGABYTE(1));

    g_State* state = mem_arenaPushStruct(mainArena, g_State);
    state->arena = mainArena;
    app_setUserData(state);

    state->window = app_makeWindow(&(app_WindowDesc) {
        .title  = s8("Sampe texture"),
        .width  = 375,
        .height = 668
    });

    rx_setup(&(rx_SetupDesc) {
        .context.gl.appleCaOpenGlLayer = app_getGraphicsHandle(state->window),
        .sampleCount = 1
    });

    // buffers

    f32 vertexData[] = {
        // positions            // colors
         0.0f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f
    };

    state->vertexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(vertexData)
    });

    rx_updateBuffer(state->vertexBuffer, 0, (rx_Range) {
        .size = sizeOf(vertexData),
        .content = vertexData
    });

    // shader

    rx_renderShader triangleShader = rx_makeRenderShader(&(rx_RenderShaderDesc) {
        .label = s8("TriangleShader"),
        .vs.source =
            s8(
            "#version 330\n"
            "layout(location=0) in vec4 position;\n"
            "layout(location=1) in vec2 texcoords0;\n"
            "out vec4 color;\n"
            "out vec2 uvs;\n"
            "layout (std140) uniform resGroup1 { float offset; };"
            "void main() {\n"
            "  gl_Position = position + vec4(offset, 0, 0, 0);\n"
            "  color = color0;\n"
            "}\n"
            ),
        .fs.source =
            s8(
            "#version 330\n"
            "in vec4 color;\n"
            "in vec2 uvs;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "  frag_color = color;\n"
            "}\n"
            )
    });

    // ResGroupLayout
    state->resGroupLayout = rx_makeResGroupLayout(&(rx_ResGroupLayoutDesc) {
        .uniformSize = sizeof(f32)
    });

    // pipeline

    state->renderPipeline = rx_makeRenderPipeline(&(rx_RenderPipelineDesc) {
        .label = s8("TriangleShader"),
        .program.shader = triangleShader,
        .rasterizer.cullMode = rx_cullMode_back,
        .primitiveTopology = rx_primitiveTopology_triangleList,
        // pos
        .layout.attrs[0] = {
            .format = rx_vertexFormat_f32x3,
        },
        // color
        .layout.attrs[1] = {
            .format = rx_vertexFormat_f32x4,
        },
    });

    f32 offset = 0;
    state->resGroup = rx_makeResGroup(&(rx_ResGroupDesc) {
        .layout = state->resGroupLayout,
        .usage  = rx_resGroupUsage_streaming,
        .initalContent = {
            .uniformContent = {
                .size = sizeOf(offset),
                .content = &offset
            }
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

    Vec2i windowSize = app_getWindowSize(state->window);
    
    rx_texture texture = rx_getCurrentSwapTexture();



    rx_renderPass renderPass = rx_makeRenderPass(&(rx_RenderPassDesc) {
        .colorTargets[0].target = texture,
        .colorTargets[0].clearColor = rgba_red,
        // Only needed on legacy apis (OpenGL/WebGL) for the "default pass"
        .width = windowSize.x,
        .height = windowSize.y
    }, NULL);


    static f32 offset;
    rx_updateResGroup(state->resGroup, &(rx_ResGroupUpdateDesc) {
        .uniformContent = {
            .content = &offset,
            .size = sizeOf(offset)
        }
    });

    mem_scoped(tmpMem, state->arena) {
        rx_RenderCmdBuilder cmdBuilder;
        rx_renderCmdBuilderInit(tmpMem.arena, &cmdBuilder, 100);
        rx_renderCmdBuilderSetPipeline(&cmdBuilder, state->renderPipeline);
        rx_renderCmdBuilderSetVertexBuffer0(&cmdBuilder, state->vertexBuffer);
        rx_renderCmdBuilderSetResGroup1(&cmdBuilder, state->resGroup);
        rx_renderCmdBuilderDraw(&cmdBuilder, 0, 6, 0, 0);

        rx_DrawArea arenas = {
            .drawCount = 1
        };

        // set cmds to render pass
        rx_setRenderPassDrawList(renderPass, &arenas, 1, cmdBuilder.drawList);
        
        // finish the frame
        rx_commit();
    }

    offset += 0.015;

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

    return 0;
}