#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "os/os.h"
#include "app/app.h"
#include "rx/rx.h"
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
    rx_buffer indexBuffer;
} g_State;

void g_init(void) {
    BaseMemory baseMem = os_getBaseMemory();
    Arena* mainArena = mem_makeArena(&baseMem, MEGABYTE(1));

    g_State* state = mem_arenaPushStruct(mainArena, g_State);
    state->arena = mainArena;
    app_setUserData(state);

    state->window = app_makeWindow(&(app_WindowDesc) {
        .title  = s8("Triangle"),
        .width  = 375,
        .height = 668
    });

    rx_setup(&(rx_SetupDesc) {
        .context.gl.appleCaOpenGlLayer = app_getGraphicsHandle(state->window),
        .sampleCount = 1
    });

    u32 vertexData[] = {2};

    state->vertexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_vertex,
        .size = sizeOf(vertexData)
    });

    rx_updateBuffer(state->vertexBuffer, 0, (rx_Range) {
        .size = sizeOf(vertexData),
        .ptr = vertexData
    });


    u16 indexData[] = {2, 2, 2};

    state->indexBuffer = rx_makeBuffer(&(rx_BufferDesc) {
        .usage = rx_bufferUsage_index,
        .size = sizeOf(indexData)
    });

    rx_updateBuffer(state->indexBuffer, 0, (rx_Range) {
        .size = sizeOf(indexData),
        .ptr = indexData
    });

    // pass def

    // shader

    // pipeline

    // show window!

    app_showWindow(state->window);
}

void g_event(app_AppEvent* event) {
    g_State* state = (g_State*) app_getUserData();

}

void g_update(void) {
    g_State* state = (g_State*) app_getUserData();

    // rx_nextFrame()
    // get next backbuffer texture
    // rx_makePass()
    // record cmds
    // present backbuffer texture

}

void g_cleanup(void) {
    g_State* state = (g_State*) app_getUserData();


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