#include "base/base.h"
#include "base/base_types.h"
#include "base/base_str.h"
#include "base/base_mem.h"
#include "base/base_math.h"

#include "os/os.h"
#include "log/log.h"
#include "app/app.h"

#if os_DlL_HOST
Str8 app__useDll(void) {
    return str8(OS_DLL_PATH);
}
#else

typedef struct app__Ctx {
    Arena* arena;
} app__Ctx;

static app__Ctx* app__ctx;

LOCAL void app__initApp(app__Ctx* ctx, app_ApplicationDesc* appDesc) {
    BaseMemory mem = os_getBaseMemory();
    Arena* mainAppArena = mem_makeArena(&mem, MEGABYTE(2));
    ctx->arena = mainAppArena;
}

LOCAL void app__uninitApp(app__Ctx* ctx, app_ApplicationDesc* appDesc) {
    ASSERT(!"Not implemented!");
}

#if OS_APPLE
#if OS_OSX
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#else
#import <UIKit/UIKit.h>
#endif

#import <QuartzCore/CAMetalLayer.h>


#ifdef OS_DLLHOST
#include "app__dllhost.h"

#define DMON_IMPL
#include "dmon.h"

#endif

#if __has_feature(objc_arc)
#define OS_OBJC_RELESE(obj) { obj = nil; }
#else
#define OS_OBJC_RELESE(obj) { [obj release]; obj = nil; }
#endif

#if OS_OSX
@interface OsApp : NSApplication
@end
@interface OsAppDelegate : NSObject<NSApplicationDelegate>
@end
@interface AppWindowDelegate : NSObject<NSWindowDelegate>
@end
//static NSWindow* window;
#define AppleWindow NSWindow
#else
@interface OsAppDelegate : NSObject<UIApplicationDelegate>
@end
//static UIWindow* window;
#define AppleWindow UIWindow
#endif

@class AppleWindow;

@interface AppView : NSView
-(CALayer*) makeBackingLayer;
@property(nonatomic, strong) CALayer* caLayer;
@property(nonatomic, strong) NSTrackingArea* trackingArea;
@end

@interface MetalAppView : AppView
@end

@interface GlAppView : AppView
@end


@interface AppWindow : AppleWindow
@property(nonatomic, strong) AppView*      gfxView;
@property(nonatomic)         u32           windowId;
@property(nonatomic)         u32           frameBufferWidth;
@property(nonatomic)         u32           frameBufferHeight;
@property(nonatomic)         u32           windowWidth;
@property(nonatomic)         u32           windowHeight;
@property(nonatomic)         f32           dpiScale;
@property(nonatomic)         bx            firstFrame;
@property(nonatomic)         bx            fullscreen;
@end

/////////////////////
// Apple context

typedef struct app__AppThreadCtx {
    BaseMemory baseMem;
    bx initialized;
    Arena* arenas[2];
} app__AppThreadCtx;

typedef struct app__AppleCtx {
    Arena* mainArena;
    void (*rx__appleCallBeforeDone)(void);
    bx started;
    bx initCalled;
    bx cleanupCalled;
    bx eventConsumed;
    bx suspended;
    u64 frameCount;
    f32 dpiScale;
    f32 windowWidth;
    f32 windowHeight;
    f32 frameBufferWidth;
    f32 frameBufferHeight;
    Str8 dllFileName;
    Str8 dllFullPath;
    os_Dl* clientDll;
    u64 dllIdx;
    bx loadNewClientDll;
    app_ApplicationDesc  desc;
    NSMutableArray<AppWindow*>*     windows;
    OsAppDelegate*      delegate;
    CVDisplayLinkRef    displayLink;
    app_window focusedWindow;
    struct {
        Vec2    delta;
        Vec2    pos;
        Vec2    scroll;
        bx      locked;
        bx      posValid;
        flags32  buttons;
    } mouse;
    flags32 flagsChangedStore;
    app_AppEvent event;
    app_keyCode keyCodes[app_keyCodes__count];

} app__AppleCtx;

LOCAL bx app__isDll;
LOCAL app__AppleCtx* app__appleCtx;
static THREAD_LOCAL app__AppThreadCtx app__appThreadCtx;

///////////////////
// Entry

//#ifdef OS_RELOAD

#ifdef OS_DLLHOST

static void app__watchCallback(dmon_watch_id watch_id, dmon_action action, const char* rootdir, const char* filepath, const char* oldfilepath, void* user) {
    (void)(user);
    (void)(watch_id);

    switch (action) {
    case DMON_ACTION_CREATE:
        // printf("CREATE: [%s]%s\n", rootdir, filepath);
        break;
    case DMON_ACTION_DELETE:
        // printf("DELETE: [%s]%s\n", rootdir, filepath);
        break;
    case DMON_ACTION_MODIFY:
        // printf("MODIFY: [%s]%s\n", rootdir, filepath);
        break;
    case DMON_ACTION_MOVE:
        // printf("MOVE: [%s]%s -> [%s]%s\n", rootdir, oldfilepath, rootdir, filepath);
        break;
    }
}

void app__hotReload(void) {
    // os_Dl* os_dlOpen(Str8 filePath);
    // void   os_dlClose(os_Dl* handle);
    // void*  os_DlSym(os_Dl* handle, const char* symbol);
    os_Dl* dllHandle = os_dlOpen(str8("foo/bar.dll"));
    void* entry = os_DlSym(dllHandle, "app__appDllEntry");


    dmon_init();
    // puts("waiting for changes ..");
    //dmon_watch_id watchId = dmon_watch(argv[1], app__watchCallback, DMON_WATCHFLAGS_RECURSIVE, NULL);
    // getchar();
    dmon_deinit();
}
#endif

// API os_Dl* os_dlOpen(Str8 filePath);
// API void   os_dlClose(os_Dl* handle);
// API void*  os_DlSym(os_Dl* handle, const char* symbol);

#ifndef OS_NO_ENTRY


typedef i32(app__dllMainFn)(app__AppleCtx* ctx, int argc, char* argv[]);
typedef void(app__dllContinueFn)(app__AppleCtx* ctx);
#ifdef OS_DLLCLIENT
extern i32 app__dllMain(app__AppleCtx* ctx, int argc, char* argv[]) {
    app__isDll = true;
    app__appleCtx = ctx;
    i32 val = app_main(argc, argv);
    return val;
}

extern void app__dllContinue(app__AppleCtx* ctx) {
    app__isDll = true;
    app__appleCtx = ctx;
    app_continue();
}
#endif

LOCAL bx app__clientDllCloseAndLoad(Arena* tmpArena) {
    if (app__appleCtx->clientDll) {
        log_trace(tmpArena, str8("dll close last"));
        os_dlClose(app__appleCtx->clientDll);
        app__appleCtx->clientDll = NULL;
    }
    bx success = true;
    os_Dl* dllHandle = NULL;
    mem_scoped(tempMem, tmpArena) {
        i32 oldIdx = app__appleCtx->dllIdx;
        app__appleCtx->dllIdx += 1;
        i32 idx = app__appleCtx->dllIdx;
        Str8 oldTargetFileName = str_join(tempMem.arena, app__appleCtx->dllFullPath, str8("_"), oldIdx, str8(".dylib"));
        Str8 targetFileName = str_join(tempMem.arena, app__appleCtx->dllFullPath, str8("_"), idx, str8(".dylib"));
        Str8 targetFileNameB = str_join(tempMem.arena, app__appleCtx->dllFullPath, str8("_"), idx);
        Str8 dllFullPath = str_join(tempMem.arena, app__appleCtx->dllFullPath, str8(".dylib"));

        // delete target dll file if it existed previously
        if (os_fileExists(targetFileName)) {
            log_trace(tempMem.arena, str8("dll delete:"), targetFileName);
            os_fileDelete(targetFileName);
            // ASSERT(!os_fileExists(targetFileName));
        }
        if (os_fileExists(oldTargetFileName)) {
            log_trace(tempMem.arena, str8("dll delete:"), oldTargetFileName);
            os_fileDelete(oldTargetFileName);
            // ASSERT(!os_fileExists(targetFileName));
        }
        log_trace(tempMem.arena, str8("dll read new:"), dllFullPath);
        Str8 dllData = os_fileRead(tempMem.arena, dllFullPath);
        if (!dllData.content) {
            success = false;
            continue;
        }
        bx success = os_fileWrite(targetFileName, dllData);
        log_trace(tempMem.arena, str8("dll write:"), targetFileName);
        if (!success) {
            success = false;
            continue;
        }
        app__appleCtx->clientDll = os_dlOpen(targetFileNameB);
        log_trace(tempMem.arena, str8("dll load:"), targetFileName);
        if (!app__appleCtx->clientDll) {
            success = false;
        }
    }
    return success;
}

#ifdef OS_DLLHOST
LOCAL void app__dmonFileWatchCallback(dmon_watch_id watch_id, dmon_action action, const char* rootdir, const char* filepath, const char* oldfilepath, app__AppleCtx* user) {
    if ( action == DMON_ACTION_CREATE || action == DMON_ACTION_MODIFY) {
        Str8 filePath = str_fromNullTerminatedCharPtr(filepath);
        if (str_hasSuffix(filePath, str8(".dylib")) && str_isEqual(str_to(filePath, filePath.size - str8(".dylib").size), user->dllFileName)) {
            user->loadNewClientDll = true;
        }
    }

}

LOCAL void app_appleStartApplication(void);
LOCAL i32 app__hostStart(int argc, char* argv[]) {
    Str8 dllFile = app__useDll();
    app__appleCtx->dllFullPath = dllFile;
    app__appleCtx->dllFileName = str_from(dllFile, str_lastIndexOfChar(dllFile, '/') + 1);

    if (!app__clientDllCloseAndLoad(app__appleCtx->mainArena)) {
        //app_log(str8("No Dll to load!"));
        return 1;
    }

    os_Dl* dllHandle = app__appleCtx->clientDll;
    app__dllMainFn* dllMainFn = os_DlSym(dllHandle, "app__dllMain");
    if (dllMainFn == NULL) {
        // error!
        //app_log(str8("No entry point in client dll! (app__dllMain)"));
        return 1;
    }

    i32 val = dllMainFn(app__appleCtx, argc, argv);

    if (val != 0) {
        // some error happened, early exit
        return val;
    }
    // we want to call the host app initializer so we overwrite it for this case
    if (app__appleCtx->rx__appleCallBeforeDone) {
        app__appleCtx->rx__appleCallBeforeDone = app_appleStartApplication;
    }
    return 0;
    //dmon_watch()

#if 0
DMON_API_DECL  dmon_watch_id dmon_watch(const char* rootdir,
                         void (*watch_cb)(dmon_watch_id watch_id, dmon_action action,
                                          const char* rootdir, const char* filepath,
                                          const char* oldfilepath, void* user),
                         uint32_t flags, void* user_data);
DMON_API_DECL void dmon_unwatch(dmon_watch_id id);
#endif
}

int app__hostEnd(void) {
    dmon_deinit();
}
#endif


int main(int argc, char* argv[]) {

#if 0

#endif
    
    // HACK: we need argument buffers for molten vulkan... 
    setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", true);
    i32 val = 0;
#ifdef OS_DLLHOST
    val = app__hostStart(argc, argv);
#else
    val = app_main(argc, argv);
#endif
    if (val != 0) {
        return val;
    }

    if (app__appleCtx->rx__appleCallBeforeDone) {
        app__appleCtx->rx__appleCallBeforeDone();
    }
    return 0;
}
#endif
void* app_getUserData(void) {
    return app__appleCtx->desc.user;
}

void app_setUserData(void* userPtr) {
    app__appleCtx->desc.user = userPtr;
}

LOCAL bx app__eventsEnabled(void) {
    /* only send events when an event callback is set, and the init function was called */
    return app__appleCtx->desc.event && app__appleCtx->initCalled;
}

LOCAL bx app__callEvent(app_AppEvent* e) {
    if (!app__appleCtx->cleanupCalled) {
        if (app__appleCtx->desc.event) {
            app__appleCtx->desc.event(e);
        }
    }
    if (app__appleCtx->eventConsumed) {
        app__appleCtx->eventConsumed = false;
        return true;
    }
    return false;
}

LOCAL void app__initEvent(app_appEventType type) {
    mem_setZero(&app__appleCtx->event, sizeof(app__appleCtx->event));
    app__appleCtx->event.type = type;
    app__appleCtx->event.window = app__appleCtx->focusedWindow;
    app__appleCtx->event.frameCount = app__appleCtx->frameCount;
    app__appleCtx->event.mouse.button = app_mouseButton__invalid;
    app__appleCtx->event.windowSize.x = app__appleCtx->windowWidth;
    app__appleCtx->event.windowSize.y = app__appleCtx->windowHeight;
    app__appleCtx->event.frameBufferSize.x = app__appleCtx->frameBufferWidth;
    app__appleCtx->event.frameBufferSize.y = app__appleCtx->frameBufferHeight;
    app__appleCtx->event.mouse.pos = app__appleCtx->mouse.pos;
    //app__appleCtx->event.mouse.pos.y = app__appleCtx->mouse.pos.y;
    app__appleCtx->event.mouse.delta = app__appleCtx->mouse.delta;
    //app__appleCtx->event.mouse.delta.y = app__appleCtx->mouse.delta.y;
}

LOCAL void app__initAndCallEvent(app_appEventType type) {
    if (app__eventsEnabled()) {
        app__initEvent(type);
        app__callEvent(&app__appleCtx->event);
    }
}

LOCAL u32 app__inputMods(NSEvent* ev) {
    const NSEventModifierFlags f = ev.modifierFlags;
    const NSUInteger b = NSEvent.pressedMouseButtons;
    u32 m = 0;
    if (f & NSEventModifierFlagShift) {
        m |= app_inputModifier_shift;
    }
    if (f & NSEventModifierFlagControl) {
        m |= app_inputModifier_ctrl;
    }
    if (f & NSEventModifierFlagOption) {
        m |= app_inputModifier_alt;
    }
    if (f & NSEventModifierFlagCommand) {
        m |= app_inputModifier_super;
    }
    if (0 != (b & (1<<0))) {
        m |= app_inputModifier_leftMouseButton;
    }
    if (0 != (b & (1<<1))) {
        m |= app_inputModifier_rightMuseButton;
    }
    if (0 != (b & (1<<2))) {
        m |= app_inputModifier_middleMouseButton;
    }
    return m;
}

LOCAL app_keyCode app__translateKey(i32 scanCode) {
    if ((scanCode >= 0) && (scanCode < app_keyCodes__count)) {
        return app__appleCtx->keyCodes[scanCode];
    }

    return app_keyCode_invalid;
}

LOCAL void app__initKeyTable(void) {
    app__appleCtx->keyCodes[0x1D] = app_keyCode_0;
    app__appleCtx->keyCodes[0x12] = app_keyCode_1;
    app__appleCtx->keyCodes[0x13] = app_keyCode_2;
    app__appleCtx->keyCodes[0x14] = app_keyCode_3;
    app__appleCtx->keyCodes[0x15] = app_keyCode_4;
    app__appleCtx->keyCodes[0x17] = app_keyCode_5;
    app__appleCtx->keyCodes[0x16] = app_keyCode_6;
    app__appleCtx->keyCodes[0x1A] = app_keyCode_7;
    app__appleCtx->keyCodes[0x1C] = app_keyCode_8;
    app__appleCtx->keyCodes[0x19] = app_keyCode_9;
    app__appleCtx->keyCodes[0x00] = app_keyCode_A;
    app__appleCtx->keyCodes[0x0B] = app_keyCode_B;
    app__appleCtx->keyCodes[0x08] = app_keyCode_C;
    app__appleCtx->keyCodes[0x02] = app_keyCode_D;
    app__appleCtx->keyCodes[0x0E] = app_keyCode_E;
    app__appleCtx->keyCodes[0x03] = app_keyCode_F;
    app__appleCtx->keyCodes[0x05] = app_keyCode_G;
    app__appleCtx->keyCodes[0x04] = app_keyCode_H;
    app__appleCtx->keyCodes[0x22] = app_keyCode_I;
    app__appleCtx->keyCodes[0x26] = app_keyCode_J;
    app__appleCtx->keyCodes[0x28] = app_keyCode_K;
    app__appleCtx->keyCodes[0x25] = app_keyCode_L;
    app__appleCtx->keyCodes[0x2E] = app_keyCode_M;
    app__appleCtx->keyCodes[0x2D] = app_keyCode_N;
    app__appleCtx->keyCodes[0x1F] = app_keyCode_O;
    app__appleCtx->keyCodes[0x23] = app_keyCode_P;
    app__appleCtx->keyCodes[0x0C] = app_keyCode_Q;
    app__appleCtx->keyCodes[0x0F] = app_keyCode_R;
    app__appleCtx->keyCodes[0x01] = app_keyCode_S;
    app__appleCtx->keyCodes[0x11] = app_keyCode_T;
    app__appleCtx->keyCodes[0x20] = app_keyCode_U;
    app__appleCtx->keyCodes[0x09] = app_keyCode_V;
    app__appleCtx->keyCodes[0x0D] = app_keyCode_W;
    app__appleCtx->keyCodes[0x07] = app_keyCode_X;
    app__appleCtx->keyCodes[0x10] = app_keyCode_Y;
    app__appleCtx->keyCodes[0x06] = app_keyCode_Z;
    app__appleCtx->keyCodes[0x27] = app_keyCode_apostrophe;
    app__appleCtx->keyCodes[0x2A] = app_keyCode_backslash;
    app__appleCtx->keyCodes[0x2B] = app_keyCode_comma;
    app__appleCtx->keyCodes[0x18] = app_keyCode_equal;
    app__appleCtx->keyCodes[0x32] = app_keyCode_gravenAccent;
    app__appleCtx->keyCodes[0x21] = app_keyCode_leftBracket;
    app__appleCtx->keyCodes[0x1B] = app_keyCode_minus;
    app__appleCtx->keyCodes[0x2F] = app_keyCode_period;
    app__appleCtx->keyCodes[0x1E] = app_keyCode_rightBracket;
    app__appleCtx->keyCodes[0x29] = app_keyCode_semicolon;
    app__appleCtx->keyCodes[0x2C] = app_keyCode_slash;
    app__appleCtx->keyCodes[0x0A] = app_keyCode_world1;
    app__appleCtx->keyCodes[0x33] = app_keyCode_backspace;
    app__appleCtx->keyCodes[0x39] = app_keyCode_capsLock;
    app__appleCtx->keyCodes[0x75] = app_keyCode_delete;
    app__appleCtx->keyCodes[0x7D] = app_keyCode_down;
    app__appleCtx->keyCodes[0x77] = app_keyCode_end;
    app__appleCtx->keyCodes[0x24] = app_keyCode_enter;
    app__appleCtx->keyCodes[0x35] = app_keyCode_escape;
    app__appleCtx->keyCodes[0x7A] = app_keyCode_F1;
    app__appleCtx->keyCodes[0x78] = app_keyCode_F2;
    app__appleCtx->keyCodes[0x63] = app_keyCode_F3;
    app__appleCtx->keyCodes[0x76] = app_keyCode_F4;
    app__appleCtx->keyCodes[0x60] = app_keyCode_F5;
    app__appleCtx->keyCodes[0x61] = app_keyCode_F6;
    app__appleCtx->keyCodes[0x62] = app_keyCode_F7;
    app__appleCtx->keyCodes[0x64] = app_keyCode_F8;
    app__appleCtx->keyCodes[0x65] = app_keyCode_F9;
    app__appleCtx->keyCodes[0x6D] = app_keyCode_F10;
    app__appleCtx->keyCodes[0x67] = app_keyCode_F11;
    app__appleCtx->keyCodes[0x6F] = app_keyCode_F12;
    app__appleCtx->keyCodes[0x69] = app_keyCode_F13;
    app__appleCtx->keyCodes[0x6B] = app_keyCode_F14;
    app__appleCtx->keyCodes[0x71] = app_keyCode_F15;
    app__appleCtx->keyCodes[0x6A] = app_keyCode_F16;
    app__appleCtx->keyCodes[0x40] = app_keyCode_F17;
    app__appleCtx->keyCodes[0x4F] = app_keyCode_F18;
    app__appleCtx->keyCodes[0x50] = app_keyCode_F19;
    app__appleCtx->keyCodes[0x5A] = app_keyCode_F20;
    app__appleCtx->keyCodes[0x73] = app_keyCode_home;
    app__appleCtx->keyCodes[0x72] = app_keyCode_insert;
    app__appleCtx->keyCodes[0x7B] = app_keyCode_left;
    app__appleCtx->keyCodes[0x3A] = app_keyCode_leftAlt;
    app__appleCtx->keyCodes[0x3B] = app_keyCode_leftControl;
    app__appleCtx->keyCodes[0x38] = app_keyCode_leftShift;
    app__appleCtx->keyCodes[0x37] = app_keyCode_leftSuper;
    app__appleCtx->keyCodes[0x6E] = app_keyCode_menu;
    app__appleCtx->keyCodes[0x47] = app_keyCode_numLock;
    app__appleCtx->keyCodes[0x79] = app_keyCode_pageDown;
    app__appleCtx->keyCodes[0x74] = app_keyCode_pageUp;
    app__appleCtx->keyCodes[0x7C] = app_keyCode_right;
    app__appleCtx->keyCodes[0x3D] = app_keyCode_rightAlt;
    app__appleCtx->keyCodes[0x3E] = app_keyCode_rightControl;
    app__appleCtx->keyCodes[0x3C] = app_keyCode_rightShift;
    app__appleCtx->keyCodes[0x36] = app_keyCode_rightSuper;
    app__appleCtx->keyCodes[0x31] = app_keyCode_space;
    app__appleCtx->keyCodes[0x30] = app_keyCode_tab;
    app__appleCtx->keyCodes[0x7E] = app_keyCode_up;
    app__appleCtx->keyCodes[0x52] = app_keyCode_keypad0;
    app__appleCtx->keyCodes[0x53] = app_keyCode_keypad1;
    app__appleCtx->keyCodes[0x54] = app_keyCode_keypad2;
    app__appleCtx->keyCodes[0x55] = app_keyCode_keypad3;
    app__appleCtx->keyCodes[0x56] = app_keyCode_keypad4;
    app__appleCtx->keyCodes[0x57] = app_keyCode_keypad5;
    app__appleCtx->keyCodes[0x58] = app_keyCode_keypad6;
    app__appleCtx->keyCodes[0x59] = app_keyCode_keypad7;
    app__appleCtx->keyCodes[0x5B] = app_keyCode_keypad8;
    app__appleCtx->keyCodes[0x5C] = app_keyCode_keypad9;
    app__appleCtx->keyCodes[0x45] = app_keyCode_keypadAdd;
    app__appleCtx->keyCodes[0x41] = app_keyCode_keypadDecimal;
    app__appleCtx->keyCodes[0x4B] = app_keyCode_keypadDivide;
    app__appleCtx->keyCodes[0x4C] = app_keyCode_keypadEnter;
    app__appleCtx->keyCodes[0x51] = app_keyCode_keypadEqual;
    app__appleCtx->keyCodes[0x43] = app_keyCode_keypadMultiply;
    app__appleCtx->keyCodes[0x4E] = app_keyCode_keypadSubtract;
}


///////////////////
// Input

#pragma mark Mouse

void app_lockMouse(bx lock) {
    if (lock == app__appleCtx->mouse.locked) {
        return;
    }
    app__appleCtx->mouse.delta.x = 0.0f;
    app__appleCtx->mouse.delta.y = 0.0f;
    app__appleCtx->mouse.locked = lock;
    /*
        NOTE that this code doesn't warp the mouse cursor to the window
        center as everybody else does it. This lead to a spike in the
        *second* mouse-moved event after the warp happened. The
        mouse centering doesn't seem to be required (mouse-moved events
        are reported correctly even when the cursor is at an edge of the screen).
        NOTE also that the hide/show of the mouse cursor should properly
        stack with calls to sapp_show_mouse()
    */
    if (app__appleCtx->mouse.locked) {
        CGAssociateMouseAndMouseCursorPosition(NO);
        [NSCursor hide];
    } else {
        [NSCursor unhide];
        CGAssociateMouseAndMouseCursorPosition(YES);
    }
}

bx app_isMouseLocked(void) {
    return app__appleCtx->mouse.locked;
}

LOCAL void app__mouseEvents(app_appEventType type, app_mouseButton btn, u32 mod) {
    if (app__eventsEnabled()) {
        app__initEvent(type);
        app__appleCtx->event.mouse.button = btn;
        app__appleCtx->event.modifiers = mod;
        app__callEvent(&app__appleCtx->event);
    }
}

LOCAL void app__updateMouse(NSEvent* event) {
    if (!app__appleCtx->mouse.locked) {
        AppWindow* window = (AppWindow*) event.window;
        const NSPoint mousePos = event.locationInWindow;
        f32 dpiScale = window.dpiScale;
        f32 newX = mousePos.x * dpiScale;
        f32 newY = app__appleCtx->frameBufferHeight - (mousePos.y * dpiScale) - 1;
        //log_trace(str8("native mouse x"), newX, str8(" y: "), newY);
        /* don't update dx/dy in the very first update */
        if (app__appleCtx->mouse.posValid) {
            app__appleCtx->mouse.delta.x = newX - app__appleCtx->mouse.pos.x;
            app__appleCtx->mouse.delta.y = newY - app__appleCtx->mouse.pos.y;
        }
        app__appleCtx->mouse.pos.x = newX;
        app__appleCtx->mouse.pos.y = newY;
        app__appleCtx->mouse.posValid = true;
    }
}

#pragma mark Mouse

LOCAL void app__keyEvent(app_appEventType type, app_keyCode key, bx repeat, u32 mod) {
    if (app__eventsEnabled()) {
        app__initEvent(type);
        app__appleCtx->event.keyCode = key;
        app__appleCtx->event.keyRepeat = repeat;
        app__appleCtx->event.modifiers = mod;
        app__callEvent(&app__appleCtx->event);
    }
}

#pragma mark Window

/* NOTE: unlike the iOS version of this function, the macOS version
    can dynamically update the DPI scaling factor when a window is moved
    between HighDPI / LowDPI screens.
*/
LOCAL void app__updateDimesion(AppWindow* appleWindow) {
    if (app__appleCtx->desc.highDpi) {
        appleWindow.dpiScale = [appleWindow screen].backingScaleFactor;
    } else {
        appleWindow.dpiScale = 1.0f;
    }

    const NSRect bounds = [appleWindow.gfxView bounds];


    appleWindow.windowWidth = bounds.size.width; //(i32) f32_round(bounds.size.width);
    appleWindow.windowHeight = bounds.size.height; //(i32) f32_round(bounds.size.height);

    appleWindow.frameBufferWidth = (int)roundf(bounds.size.width * appleWindow.dpiScale);
    appleWindow.frameBufferHeight = (int)roundf(bounds.size.height * appleWindow.dpiScale);

    app__appleCtx->frameBufferWidth = appleWindow.frameBufferWidth;
    app__appleCtx->frameBufferHeight = appleWindow.frameBufferHeight;
    app__appleCtx->windowWidth = appleWindow.windowWidth;
    app__appleCtx->windowHeight = appleWindow.windowHeight;


    const CGSize fb_size = appleWindow.gfxView.caLayer.frame.size;
    const i32 cur_fb_width = (int)roundf(fb_size.width);
    const i32 cur_fb_height = (int)roundf(fb_size.height);
    const bx dim_changed = (appleWindow.frameBufferWidth != cur_fb_width) || (appleWindow.frameBufferHeight != cur_fb_height);
    if (appleWindow.frameBufferWidth == 0) {
        appleWindow.frameBufferWidth = 1;
    }
    if (appleWindow.frameBufferHeight == 0) {
        appleWindow.frameBufferHeight = 1;
    }
    if (appleWindow.windowWidth == 0) {
        appleWindow.windowWidth = 1;
    }
    if (appleWindow.windowHeight == 0) {
        appleWindow.windowHeight = 1;
    }
    if (dim_changed) {
        CGSize drawable_size = { (f32) appleWindow.frameBufferWidth, (f32) appleWindow.frameBufferHeight };
        CGRect frame = appleWindow.gfxView.frame;
        frame.size = drawable_size;
        appleWindow.gfxView.frame = frame;
        if (!appleWindow.firstFrame) {
            app__appleCtx->event.window.id = appleWindow.windowId;
            app__appleCtx->frameBufferWidth = appleWindow.frameBufferWidth;
            app__appleCtx->frameBufferHeight = appleWindow.frameBufferHeight;
            app__appleCtx->windowWidth = appleWindow.windowWidth;
            app__appleCtx->windowHeight = appleWindow.windowHeight;
            app__initEvent(app_appEventType_resized);
            app__callEvent(&app__appleCtx->event);
        }
    }
}

#if 0

LOCAL void app__updateMouse(NSEvent* event) {
    if (!_sapp.mouse.locked) {
        const NSPoint mouse_pos = event.locationInWindow;
        float new_x = mouse_pos.x * _sapp.dpi_scale;
        float new_y = _sapp.framebuffer_height - (mouse_pos.y * _sapp.dpi_scale) - 1;
        /* don't update dx/dy in the very first update */
        if (_sapp.mouse.papp_valid) {
            _sapp.mouse.dx = new_x - _sapp.mouse.x;
            _sapp.mouse.dy = new_y - _sapp.mouse.y;
        }
        _sapp.mouse.x = new_x;
        _sapp.mouse.y = new_y;
        _sapp.mouse.papp_valid = true;
    }
}

LOCAL void app__showMouse(bx visible) {
    /* NOTE: this function is only called when the mouse visibility actually changes */
    if (visible) {
        CGDisplayShowCursor(kCGDirectMainDisplay);
    }
    else {
        CGDisplayHideCursor(kCGDirectMainDisplay);
    }
}

LOCAL void _sapp_macapp_lock_mouse(bool lock) {
    if (lock == _sapp.mouse.locked) {
        return;
    }
    _sapp.mouse.dx = 0.0f;
    _sapp.mouse.dy = 0.0f;
    _sapp.mouse.locked = lock;
    /*
        NOTE that this code doesn't warp the mouse cursor to the window
        center as everybody else does it. This lead to a spike in the
        *second* mouse-moved event after the warp happened. The
        mouse centering doesn't seem to be required (mouse-moved events
        are reported correctly even when the cursor is at an edge of the screen).
        NOTE also that the hide/show of the mouse cursor should properly
        stack with calls to sapp_show_mouse()
    */
    if (_sapp.mouse.locked) {
        CGAssociateMouseAndMouseCursorPosition(NO);
        [NSCursor hide];
    }
    else {
        [NSCursor unhide];
        CGAssociateMouseAndMouseCursorPosition(YES);
    }
}

LOCAL void _sapp_macapp_update_cursor(sapp_mouse_cursor cursor, bool shown) {
    // show/hide cursor only if visibility status has changed (required because show/hide stacks)
    if (shown != _sapp.mouse.shown) {
        if (shown) {
            [NSCursor unhide];
        }
        else {
            [NSCursor hide];
        }
    }
    // update cursor type
    SOKOL_ASSERT((cursor >= 0) && (cursor < _SAPP_MOUSECURSOR_NUM));
    if (_sapp.macos.cursors[cursor]) {
        [_sapp.macos.cursors[cursor] set];
    }
    else {
        [[NSCursor arrowCursor] set];
    }
}

LOCAL void _sapp_macapp_set_icon(const sapp_icon_desc* icon_desc, int num_images) {
    NSDockTile* dock_tile = NSApp.dockTile;
    const int wanted_width = (int) dock_tile.size.width;
    const int wanted_height = (int) dock_tile.size.height;
    const int img_index = _sapp_image_bestmatch(icon_desc->images, num_images, wanted_width, wanted_height);
    const sapp_image_desc* img_desc = &icon_desc->images[img_index];

    CGColorSpaceRef cg_color_space = CGColorSpaceCreateDeviceRGB();
    CFDataRef cf_data = CFDataCreate(kCFAllocatorDefault, (const UInt8*)img_desc->pixels.ptr, (CFIndex)img_desc->pixels.size);
    CGDataProviderRef cg_data_provider = CGDataProviderCreateWithCFData(cf_data);
    CGImageRef cg_img = CGImageCreate(
        (size_t)img_desc->width,    // width
        (size_t)img_desc->height,   // height
        8,                          // bitsPerComponent
        32,                         // bitsPerPixel
        (size_t)img_desc->width * 4,// bytesPerRow
        cg_color_space,             // space
        kCGImageAlphaLast | kCGImageByteOrderDefault,  // bitmapInfo
        cg_data_provider,           // provider
        NULL,                       // decode
        false,                      // shouldInterpolate
        kCGRenderingIntentDefault);
    CFRelease(cf_data);
    CGDataProviderRelease(cg_data_provider);
    CGColorSpaceRelease(cg_color_space);

    NSImage* ns_image = [[NSImage alloc] initWithCGImage:cg_img size:dock_tile.size];
    dock_tile.contentView = [NSImageView imageViewWithImage:ns_image];
    [dock_tile display];
    _SAPP_OBJC_RELEASE(ns_image);
    CGImageRelease(cg_img);
}

#endif

///////////////////
// Application


#if OS_OSX
@interface AppWindow () <NSWindowDelegate>
#else
@interface AppWindow () <UIApplicationDelegate>
#endif
@end

static CVReturn app__appleDisplayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* target);



@implementation AppView

- (CALayer *)makeBackingLayer {
    ASSERT(!"Overwrite makeBackingLayer!");
    return nil;
}
    
/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
-(BOOL) wantsUpdateLayer {
    return YES;
}
    
- (BOOL) isOpaque {
    return YES;
}

#if OS_OSX

#if 0
- (void)reshape {
    app__updateDimesion((AppWindow*) self.window);
    [super reshape];
}
#endif

- (BOOL)canBecomeKey {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)updateTrackingAreas {
    if (self.trackingArea != nil) {
        [self removeTrackingArea:self.trackingArea];
        OS_OBJC_RELESE(self.trackingArea);
    }
    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                          NSTrackingActiveInKeyWindow |
                                          NSTrackingEnabledDuringMouseDrag |
                                          NSTrackingCursorUpdate |
                                          NSTrackingInVisibleRect |
                                          NSTrackingAssumeInside;
    self.trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:options owner:self userInfo:nil];
    [self addTrackingArea:self.trackingArea];
    [super updateTrackingAreas];
}

- (void)mouseEntered:(NSEvent*)event {
    app__updateMouse(event);
    /* don't send mouse enter/leave while dragging (so that it behaves the same as
       on Windows while SetCapture is active
    */
    if (0 == app__appleCtx->mouse.buttons) {
        app__mouseEvents(app_appEventType_mouseEnter, app_mouseButton__invalid, app__inputMods(event));
    }
}

- (void)mouseExited:(NSEvent*)event {
    app__updateMouse(event);
    if (0 == app__appleCtx->mouse.buttons) {
        app__mouseEvents(app_appEventType_mouseLeave, app_mouseButton__invalid, app__inputMods(event));
    }
}

- (void)mouseDown:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseDown, app_mouseButton_left, app__inputMods(event));
    app__appleCtx->mouse.buttons |= (1<<app_mouseButton_left);
}

- (void)mouseUp:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseUp, app_mouseButton_left, app__inputMods(event));
    app__appleCtx->mouse.buttons &= ~(1<<app_mouseButton_left);
}

- (void)rightMouseDown:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseDown, app_mouseButton_right, app__inputMods(event));
    app__appleCtx->mouse.buttons |= (1<<app_mouseButton_right);
}

- (void)rightMouseUp:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseUp, app_mouseButton_right, app__inputMods(event));
    app__appleCtx->mouse.buttons &= ~(1<<app_mouseButton_right);
}

- (void)otherMouseDown:(NSEvent*)event {
    app__updateMouse(event);
    if (2 == event.buttonNumber) {
        app__mouseEvents(app_appEventType_mouseDown, app_mouseButton_middle, app__inputMods(event));
        app__appleCtx->mouse.buttons |= (1<<app_mouseButton_middle);
    }
}

- (void)otherMouseUp:(NSEvent*)event {
    app__updateMouse(event);
    if (2 == event.buttonNumber) {
        app__mouseEvents(app_appEventType_mouseUp, app_mouseButton_middle, app__inputMods(event));
        app__appleCtx->mouse.buttons &= (1<<app_mouseButton_middle);
    }
}

- (void)otherMouseDragged:(NSEvent*)event {
    app__updateMouse(event);
    if (2 == event.buttonNumber) {
        if (app__appleCtx->mouse.locked) {
            app__appleCtx->mouse.delta.x = [event deltaX];
            app__appleCtx->mouse.delta.y = [event deltaY];
        }
        app__mouseEvents(app_appEventType_mouseMove, app_mouseButton__invalid, app__inputMods(event));
    }
}
- (void)mouseMoved:(NSEvent*)event {
    app__updateMouse(event);
    if (app__appleCtx->mouse.locked) {
        app__appleCtx->mouse.delta.x = [event deltaX];
        app__appleCtx->mouse.delta.y = [event deltaY];
    }
    app__mouseEvents(app_appEventType_mouseMove, app_mouseButton__invalid, app__inputMods(event));
}

- (void)mouseDragged:(NSEvent*)event {
    app__updateMouse(event);
    if (app__appleCtx->mouse.locked) {
        app__appleCtx->mouse.delta.x = [event deltaX];
        app__appleCtx->mouse.delta.y = [event deltaY];
    }
    app__mouseEvents(app_appEventType_mouseMove, app_mouseButton__invalid, app__inputMods(event));
}

- (void)rightMouseDragged:(NSEvent*)event {
    app__updateMouse(event);
    if (app__appleCtx->mouse.locked) {
        app__appleCtx->mouse.delta.x = [event deltaX];
        app__appleCtx->mouse.delta.y = [event deltaY];
    }
    app__mouseEvents(app_appEventType_mouseMove, app_mouseButton__invalid, app__inputMods(event));
}

#define ABSF(F) ((f32) absVal(F))
- (void)scrollWheel:(NSEvent*)event {
    app__updateMouse(event);
    if (app__eventsEnabled()) {
        float dx = (float) event.scrollingDeltaX;
        float dy = (float) event.scrollingDeltaY;
        if (event.hasPreciseScrollingDeltas) {
            dx *= 0.1;
            dy *= 0.1;
        }
        if ((ABSF(dx) > 0.0f) || (ABSF(dy) > 0.0f)) {
            app__initEvent(app_appEventType_mouseScroll);
            app__appleCtx->event.modifiers = app__inputMods(event);
            app__appleCtx->event.mouse.scroll.x = dx;
            app__appleCtx->event.mouse.scroll.y = dy;
            app__callEvent(&app__appleCtx->event);
        }
    }
}

- (void)keyDown:(NSEvent*)event {
    if (app__eventsEnabled()) {
        const uint32_t mods = app__inputMods(event);
        /* NOTE: macOS doesn't send keyUp events while the Cmd key is pressed,
            as a workaround, to prevent key presses from sticking we'll send
            a keyup event following right after the keydown if SUPER is also pressed
        */
        const app_keyCode key_code = app__translateKey(event.keyCode);
        app__keyEvent(app_appEventType_keyDown, key_code, event.isARepeat, mods);
        if (0 != (mods & app_inputModifier_super)) {
            app__keyEvent(app_appEventType_keyUp, key_code, event.isARepeat, mods);
        }
        const NSString* chars = event.characters;
        const NSUInteger len = chars.length;
        if (len > 0) {
            app__initEvent(app_appEventType_char);
            app__appleCtx->event.modifiers = mods;
            for (NSUInteger i = 0; i < len; i++) {
                const unichar codepoint = [chars characterAtIndex:i];
                if ((codepoint & 0xFF00) == 0xF700) {
                    continue;
                }
                app__appleCtx->event.charCode = codepoint;
                app__appleCtx->event.keyRepeat = event.isARepeat;
                app__callEvent(&app__appleCtx->event);
            }
        }
        /* if this is a Cmd+V (paste), also send a CLIPBOARD_PASTE event */
        #if 0
        if (app__appleCtx->clipboard.enabled && (mods == app_inputModifier_super) && (key_code == app_keyCode_V)) {
            _sapp_init_event(SAPP_EVENTTYPE_CLIPBOARD_PASTED);
            _sapp_call_event(&_sapp.event);
        }
        #endif
    }
}

- (void)keyUp:(NSEvent*)event {
    app__keyEvent(app_appEventType_keyUp,
        app__translateKey(event.keyCode),
        event.isARepeat,
        app__inputMods(event));
}

- (void)flagsChanged:(NSEvent*)event {
    const uint32_t old_f = app__appleCtx->flagsChangedStore;
    const uint32_t new_f = event.modifierFlags;
    app__appleCtx->flagsChangedStore = new_f;
    app_keyCode key_code = app_keyCode_invalid;
    bx down = false;
    if ((new_f ^ old_f) & NSEventModifierFlagShift) {
        key_code = app_keyCode_leftShift;
        down = 0 != (new_f & NSEventModifierFlagShift);
    }
    if ((new_f ^ old_f) & NSEventModifierFlagControl) {
        key_code = app_keyCode_leftControl;
        down = 0 != (new_f & NSEventModifierFlagControl);
    }
    if ((new_f ^ old_f) & NSEventModifierFlagOption) {
        key_code = app_keyCode_leftAlt;
        down = 0 != (new_f & NSEventModifierFlagOption);
    }
    if ((new_f ^ old_f) & NSEventModifierFlagCommand) {
        key_code = app_keyCode_leftSuper;
        down = 0 != (new_f & NSEventModifierFlagCommand);
    }
    if (key_code != app_keyCode_invalid) {
        app__keyEvent(down ? app_appEventType_keyDown : app_appEventType_keyUp,
            key_code,
            false,
            app__inputMods(event));
    }
}

#endif // OS_OSX

@end

@implementation MetalAppView
+ (Class) layerClass {
    return [CAMetalLayer class];
}

-(CALayer*) makeBackingLayer {
    if (self.caLayer) {
        return self.caLayer;
    }
    CALayer* layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    layer.contentsScale = minVal(viewScale.width, viewScale.height);
    self.caLayer = layer;
    return layer;
}

@end
@implementation GlAppView
#if OS_OSX
// OpenGLLayer
#define AppGLLayer NSOpenGLLayer
#else
#define AppGLLayer CAEAGLLayer
#endif
+ (Class) layerClass {
    // IOS: CAEAGLLayer
    return [AppGLLayer class];
}
- (CALayer *)makeBackingLayer {
    if (self.caLayer) {
        return self.caLayer;
    }
    AppGLLayer* glLayer = [[AppGLLayer alloc] init];
    [glLayer setAsynchronous:YES];
    self.caLayer = glLayer;
    CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    self.caLayer.contentsScale = minVal(viewScale.width, viewScale.height);
    return self.caLayer;
}

@end

@implementation AppWindow

- (void)windowDidResize:(NSNotification*)notification {
    unusedVars(notification);
    app__updateDimesion(self);
}

- (void)windowDidChangeScreen:(NSNotification*)notification {
    unusedVars(notification);
    //_sapp_timing_reset(&_sapp.timing);
    app__updateDimesion(self);
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    unusedVars(notification);
    app__initEvent(app_appEventType_iconified);
    app__appleCtx->event.window.id = self.windowId;
    app__callEvent(&app__appleCtx->event);
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    unusedVars(notification);
    app__initEvent(app_appEventType_restored);
    app__appleCtx->event.window.id = self.windowId;
    app__callEvent(&app__appleCtx->event);
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    unusedVars(notification);
    app__appleCtx->focusedWindow.id = self.windowId;
    app__initEvent(app_appEventType_focused);
    app__callEvent(&app__appleCtx->event);
}

- (void)windowDidResignKey:(NSNotification*)notification {
    unusedVars(notification);
    if (app__appleCtx->focusedWindow.id == self.windowId) {
        app__appleCtx->focusedWindow.id = 0;
    }
    app__initEvent(app_appEventType_unfocused);
    app__appleCtx->event.window.id = self.windowId;
    app__callEvent(&app__appleCtx->event);
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
    unusedVars(notification);
    self.fullscreen = true;
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
    unusedVars(notification);
    self.fullscreen = false;
}

@end

#if OS_OSX
//------------------------------------------------------------------------------
@implementation OsApp
// From http://cocoadev.com/index.pl?GameKeyboardHandlingAlmost
// This works around an AppKit bug, where key up events while holding
// down the command key don't get sent to the key window.
- (void)sendEvent:(NSEvent*) event {
    if ([event type] == NSEventTypeKeyUp && ([event modifierFlags] & NSEventModifierFlagCommand)) {
        [[self keyWindow] sendEvent:event];
    } else {
        [super sendEvent:event];
    }
}
@end

#else

#endif

@implementation OsAppDelegate
#if OS_OSX
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    (void)aNotification;
#else
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    (void)application;
    (void)launchOptions;
#endif
    // call the init function
    if (app__appleCtx->desc.init) {
        //mem_Scratch scratch = mem_scratchStart(app_tempMemory());
        app__appleCtx->desc.init();
        //mem_scratchEnd(&scratch);
    }


#ifdef OS_DLLHOST
    dmon_init();
    mem_scoped(tempMemory, app__appleCtx->mainArena) {
        i64 lastIdx = str_lastIndexOfChar(app__appleCtx->dllFullPath, '/');
        Str8 dllFolder = str_to(app__appleCtx->dllFullPath, lastIdx + 1);
        Str8 dllCharFolder = str_join(tempMemory.arena, dllFolder, '\0');
        dmon_watch((unsigned char*) dllCharFolder.content, app__dmonFileWatchCallback, 0, app__appleCtx);
    }
#endif

    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    [NSApp activateIgnoringOtherApps:YES];
    [NSEvent setMouseCoalescingEnabled:NO];
    
    app_appInitThread();

    CVDisplayLinkCreateWithActiveCGDisplays(&app__appleCtx->displayLink);
    CVDisplayLinkSetOutputCallback(app__appleCtx->displayLink, &app__appleDisplayLinkCallback, nil);
    CVDisplayLinkStart(app__appleCtx->displayLink);
#if OS_OSX
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillResignActive:) name:NSApplicationWillResignActiveNotification object:nil];
    //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:NSApplicationDidBecomeActive object:nil];
#endif
    //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];
#if OS_IOS
    return YES;
#endif
}

#if OS_OSX
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    unusedVars(sender);
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSApplicationWillResignActiveNotification object:nil];
    //[[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationWillTerminateNotification object:nil];
    return YES;
}
#endif

#if OS_IOS
- (void)applicationWillResignActive:(UIApplication *)application {
#elif OS_OSX
-(void)appWillResignActive:(NSNotification*)note {
#endif
    if (!app__appleCtx->suspended) {
        app__appleCtx->suspended = true;
        app__initAndCallEvent(app_appEventType_suspended);
    }
}

#if OS_IOS
- (void)applicationDidBecomeActive:(UIApplication *)application {
#elif OS_OSX
-(void) applicationDidBecomeActive:(NSNotification *)notification {
#endif
    if (app__appleCtx->suspended) {
        app__appleCtx->suspended = false;
        app__initAndCallEvent(app_appEventType_resumed);
    }
}

@end

/** Rendering loop callback function for use with a CVDisplayLink. */
CVReturn app__appleDisplayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* target) {
    // update can happen on a different thread so we initialize it again.
    app_appInitThread();
    {
        if (app__appleCtx->loadNewClientDll) {
            app__appleCtx->loadNewClientDll = false;
            app__appleCtx->desc.prepareReloadDll();
            if (app__clientDllCloseAndLoad(app__appleCtx->mainArena)) {

                app__dllContinueFn* dllMainFn = os_dlSym(app__appleCtx->clientDll, "app__dllContinue");
                if (dllMainFn == NULL) {
                    // error!
                    // app_log(str8("No entry point in client dll! (app__dllMain)"));
                    return 1;
                }

                dllMainFn(app__appleCtx);
            }
        }
        app__appleCtx->desc.update();
    }
    app_appNextFrameThread();
    app__appleCtx->frameCount++;
    return kCVReturnSuccess;
}

static void app__appleSetWindowName(AppWindow* window, Str8 name) {
#if OS_OSX
    u8 title[2057];
    u64 size = minVal(name.size, (sizeof(title) - 1));
    mem_copy(title, name.content, size);
    title[size] = '\0';
    [window setTitle:[NSString stringWithUTF8String:(char*) &title[0]]];
#else
    unusedVars(window);
    unusedVars(name);
#endif
}

static uint32_t app__appleCreateWindow(app_WindowDesc* desc) {
    AppWindow* window = nil;
    // window delegate and main window
    #if OS_IOS
        CGRect mainScreenBounds = [[UIScreen mainScreen] bounds];
        window = [[AppWindow alloc] initWithFrame:mainScreenBounds];
    #else
        //window.windowDelegate = [[AppWindowDelegate alloc] init];
        const NSUInteger style =
            NSWindowStyleMaskTitled |
            NSWindowStyleMaskClosable |
            NSWindowStyleMaskMiniaturizable |
            NSWindowStyleMaskResizable;
        window = [[AppWindow alloc]
            initWithContentRect:NSMakeRect(desc->x, desc->y, desc->width, desc->height)
            styleMask:style
            backing:NSBackingStoreBuffered
            defer:NO];
        [window setAcceptsMouseMovedEvents:YES];
        [window center];
        [window setRestorable:YES];
        [window setDelegate:window];
    #endif
    NSRect windowRect = NSMakeRect(0, 0, desc->width, desc->height);
#if 0
    if (desc->gfxApi == app_windowGfxApi_openGl) {
        NSOpenGLPixelFormatAttribute attrs[32];
        int i = 0;
        attrs[i++] = NSOpenGLPFAAccelerated;
        attrs[i++] = NSOpenGLPFADoubleBuffer;
        attrs[i++] = NSOpenGLPFAOpenGLProfile;
        const int glVersion = 41; //_sapp.desc.gl_major_version * 10 + _sapp.desc.gl_minor_version;
        switch(glVersion) {
            case 10: attrs[i++] = NSOpenGLProfileVersionLegacy;  break;
            case 32: attrs[i++] = NSOpenGLProfileVersion3_2Core; break;
            case 41: attrs[i++] = NSOpenGLProfileVersion4_1Core; break;
            default: ASSERT(!"MACOS_INVALID_NSOPENGL_PROFILE");//_SAPP_PANIC(MACOS_INVALID_NSOPENGL_PROFILE);
        }
        attrs[i++] = NSOpenGLPFAColorSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAAlphaSize; attrs[i++] = 8;
        attrs[i++] = NSOpenGLPFADepthSize; attrs[i++] = 24;
        attrs[i++] = NSOpenGLPFAStencilSize; attrs[i++] = 8;
        if (desc->sampleCount > 1) {
            attrs[i++] = NSOpenGLPFAMultisample;
            attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 1;
            attrs[i++] = NSOpenGLPFASamples; attrs[i++] = (NSOpenGLPixelFormatAttribute) desc->sampleCount;
        }
        else {
            attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 0;
        }
        attrs[i++] = 0;
        NSOpenGLPixelFormat* glpixelformatObj = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        ASSERT(glpixelformatObj != nil);
        GlAppView* glAppView = [[GlAppView alloc] init];
        window.gfxView = glAppView;
        OS_OBJC_RELESE(glpixelformatObj);
        window.contentView = window.gfxView;
        //[window.glView prepareOpenGL];
        ASSERT(window.glView.openGLContext);
        NSOpenGLLayer
    } else {
        window.metalView = [[AppMetalView alloc] init];

    #if OS_OSX
        window.contentView = window.metalView;
        CGSize drawable_size = { (CGFloat) desc->width, (CGFloat) desc->height };
        // [mtk_view setDrawableSize:drawable_size];
        // [[mtk_view layer] setMagnificationFilter:kCAFilterNearest];
        
        //[window.metalView setContentScaleFactor:1.0f];
        //[window.metalView setUserInteractionEnabled:YES];
        //[window.metalView setMultipleTouchEnabled:YES];
        //[window addSubview:window.metalView];
        //[window makeKeyAndVisible];
    #endif
        [window layoutIfNeeded];
        [window.gfxView setLayer:[window.gfxView makeBackingLayer]];
        [window.gfxView setWantsLayer:YES];
        [window.gfxView.layer setContentsScale:[window backingScaleFactor]];
    }
#endif

    if (desc->gfxApi == app_windowGfxApi_openGl) {
        window.gfxView = [[GlAppView alloc] init];
    } else {
        window.gfxView = [[MetalAppView alloc] init];
    }

    [window layoutIfNeeded];
    [window.gfxView setLayer:[window.gfxView makeBackingLayer]];
    [window.gfxView setWantsLayer:YES];
    [window.gfxView.layer setContentsScale:[window backingScaleFactor]];
    
    [window makeFirstResponder:window.contentView];
    app__appleSetWindowName(window, desc->title);
    [window makeKeyAndOrderFront:nil];
    [window layoutIfNeeded];
    [app__appleCtx->windows addObject:window];
    app__updateDimesion(window);


    return [app__appleCtx->windows count] - 1;
}

void app_appleStartApplication(void) {
    app__initKeyTable();
    [NSApplication sharedApplication];
    OsAppDelegate* delegate = [[OsAppDelegate alloc] init];
    app__appleCtx->delegate = delegate;
    NSApp.delegate = delegate;
    [NSApp run];
}

void app_initApplication(app_ApplicationDesc* applicationDesc) {
    BaseMemory baseMem = os_getBaseMemory();
    mms appMemoryBudget = applicationDesc->appMemoryBudget == 0 ? MEGABYTE(20) : applicationDesc->appMemoryBudget;
    Arena* mainArena = mem_makeArena(&baseMem, appMemoryBudget);
    app__appleCtx = mem_arenaPushStructZero(mainArena, app__AppleCtx);
    app__appleCtx->mainArena = mainArena;


    app__appleCtx->started = true;
    bx highDpi = app__appleCtx->desc.highDpi;
    void* user = app__appleCtx->desc.user;
    app__appleCtx->desc = *applicationDesc;
    if (!app__appleCtx->desc.highDpi) {
        app__appleCtx->desc.highDpi = highDpi;
    }
    if (!app__appleCtx->desc.user) {
        app__appleCtx->desc.user = user;
    }
    app__appleCtx->windows = [NSMutableArray array];
#ifndef OS_NO_ENTRY 
    app__appleCtx->rx__appleCallBeforeDone = app_appleStartApplication;
#endif
    app__appleCtx->initCalled = true;
}

void app_stopApplication (void) {
    ASSERT(!"Implement me!");
}

u32 app_maxWindows(void) {
#if OS_IOS
    return 1;
#else
    return 100;
#endif
}

Vec2 app_getWindowSizeF32(app_window window) {
    AppWindow* osWindow = app__appleCtx->windows[window.id - 1];
    Vec2 size;
    size.x = osWindow.frameBufferWidth;
    size.y = osWindow.frameBufferHeight;

    return size;
}

SVec2 app_getWindowSize(app_window window) {
    Vec2 fSize = app_getWindowSizeF32(window);
    SVec2 size;
    size.x = (i32) f32_round(fSize.x);
    size.y = (i32) f32_round(fSize.y);

    return size;
}

app_window app_makeWindow(app_WindowDesc* desc) {
    app_window window;
    window.id = 0;
    if ([app__appleCtx->windows count] >= app_maxWindows()) {
        return window;
    }

    window.id = app__appleCreateWindow(desc) + 1;
    return window;
}

void app_showWindow(app_window window) {
    ASSERT(window.id);
    u32 windowIndex = window.id - 1;
    AppWindow* nsWindow = app__appleCtx->windows[windowIndex];

    ASSERT(nsWindow);
#if OS_OSX
    [NSApp activateIgnoringOtherApps:YES];
    [nsWindow makeKeyAndOrderFront:nil];
    [nsWindow layoutIfNeeded];
    [nsWindow orderFrontRegardless];
#else
    [nsWindow makeKeyAndVisible];
#endif // OS_OSX
}

void app_hideWindow(app_window window) {
    ASSERT(window.id);
#if OS_OSX
    u32 windowIndex = window.id - 1;
    AppWindow* nsWindow = app__appleCtx->windows[windowIndex];
    ASSERT(nsWindow);
    [nsWindow orderOut:nsWindow];
#else
    unusedVars(&window);
#endif // OS_OSX
}

void app_destroyWindow(app_window window) {
    ASSERT(window.id);
#if OS_OSX
    u32 windowIndex = window.id - 1;
    AppWindow* nsWindow = app__appleCtx->windows[windowIndex];
    ASSERT(nsWindow);
    app_hideWindow(window);
    app__appleCtx->windows[windowIndex] = (AppWindow*) [NSNull null];
#else
    unusedVars(&window);
#endif // OS_OSX
}

void* app_getGraphicsHandle(app_window window) {
    ASSERT(window.id);
    u32 windowIndex = window.id - 1;
    AppWindow* nsWindow = app__appleCtx->windows[windowIndex];
    return (void*) [nsWindow.gfxView makeBackingLayer];
}

Arena* app_frameArena(void) {
    return app__appThreadCtx.arenas[(app__appleCtx->frameCount % 2)];
}

void app_appInitThread(void) {
    if (app__appThreadCtx.initialized == true) {
        return;
    }
    BaseMemory baseMem = os_getBaseMemory();
    for (u32 idx = 0; idx < countOf(app__appThreadCtx.arenas); idx++) {
        app__appThreadCtx.arenas[idx] = mem_makeArena(&baseMem, 20);
    }
    app__appThreadCtx.initialized = true;
}

void app_appNextFrameThread(void) {
    // reset frame arena
    mem_arenaPopTo(app__appThreadCtx.arenas[(app__appleCtx->frameCount % 2)], 0);
}

void app_appCleanupThread(void) {
    for (u32 idx = 0; idx < countOf(app__appThreadCtx.arenas); idx++) {
        mem_destroyArena(app__appThreadCtx.arenas[idx]);
        app__appThreadCtx.arenas[idx] = NULL;
    }
}

#elif OS_ANDROID
#error "ANDROID no implemented"
#elif OS_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void (*rx__winCallBeforeDone)(void);

LOCAL char** app__cmdToUtf8Argv(LPWSTR w_command_line, i32* o_argc) {
    i32 argc = 0;
    char** argv = 0;
    char* args;

    LPWSTR* w_argv = CommandLineToArgvW(w_command_line, &argc);
    if (w_argv == NULL) {
        ASSERT(!"Win32: failed to parse command line");
    } else {
        size_t size = wcslen(w_command_line) * 4;
        u64 byteSize = ((size_t)argc + 1) * sizeof(char*) + size;
        argv = (char**) mem_arenaPush(os_tempMemory(), byteSize);
        mem_setZero(argv, byteSize);
        //argv = (char**) calloc(1, ((size_t)argc + 1) * sizeof(char*) + size);
        ASSERT(argv);
        args = (char*) &argv[argc + 1];
        int n;
        for (int i = 0; i < argc; ++i) {
            n = WideCharToMultiByte(CP_UTF8, 0, w_argv[i], -1, args, (int)size, NULL, NULL);
            if (n == 0) {
                ASSERT(!"Win32: failed to convert all arguments to utf8");
                break;
            }
            argv[i] = args;
            size -= (size_t)n;
            args += n;
        }
        LocalFree(w_argv);
    }
    *o_argc = argc;
    return argv;
}

int app__main(int argc, char* argv[]) {
    os_threadInit();
    os_init();
    mem_Scratch scratch = mem_scratchStart(os_tempMemory());
    i32 val = app_main(argc, argv);
    mem_scratchEnd(&scratch);
    if (val != 0) {
        return val;
    }
    if (rx__winCallBeforeDone) {
        rx__winCallBeforeDone();
    }
    app_threadUninit();
    return 0;
}

int main(int argc, char* argv[]) {
    return app__main(argc, argv);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    unusedVars(hInstance);
    unusedVars(hPrevInstance);
    unusedVars(lpCmdLine);
    unusedVars(nCmdShow);
    os_threadInit();
    int argc_utf8 = 0;
    char** argv_utf8 = app__cmdToUtf8Argv(GetCommandLineW(), &argc_utf8);

    i32 retVal = app__main(argc_utf8, argv_utf8);
    free(argv_utf8);

    return retVal;
}

#ifndef OS_MAX_WINDOWS
#define OS_MAX_WINDOWS 100
#endif // OS_MAX_WINDOWS

typedef struct app__Win32Window {
    HWND hWnd;
    bool active;
} app__Win32Window;

typedef struct app__Win32AppCtx {
    bool initialized;
    WNDCLASSEXA windowClass;
    app_ApplicationDesc desc;
    app__Win32Window windows[OS_MAX_WINDOWS];
} app__Win32AppCtx;

static app__Win32AppCtx* app__win32AppCtx;

#ifdef OS_RELOAD
__declspec(dllexport) app__dllEntry(app__Win32AppCtx* ctx) {
    app__win32AppCtx = ctx;
}
#endif

static LRESULT DefaultWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        }
        default: break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static const char* app__Win32WindowClass = "AppWindowClass";
void app_startApplication(void) {
    WNDCLASSEXA* windowClass = &app__win32AppCtx->windowClass;
    windowClass->cbSize        = sizeof(WNDCLASSEXA);
    windowClass->cbWndExtra    = sizeof(void*);
    windowClass->lpfnWndProc   = (WNDPROC) DefaultWindowProcWin32;
    windowClass->lpszClassName = app__Win32WindowClass;
    windowClass->hCursor       = LoadCursorA(NULL, MAKEINTRESOURCEA(32512));
    windowClass->style         = CS_OWNDC | CS_DBLCLKS;
    if (!RegisterClassExA(windowClass)) {
        ASSERT(!"Failed to initialize window class.");
        return;
    }
    if (app__win32AppCtx->desc.init) {
        mem_Scratch scratch = mem_scratchStart(os_tempMemory());
        app__win32AppCtx->desc.init();
        mem_scratchEnd(&scratch);
    }
    while (1) {
        MSG msg;
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                if (app__win32AppCtx->desc.cleanup) {
                    app__win32AppCtx->desc.cleanup();
                }
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (app__win32AppCtx->desc.update) {
            mem_Scratch scratch = mem_scratchStart(app_tempMemory());
            app__win32AppCtx->desc.update();
            mem_scratchEnd(&scratch);
        }
    }
}
void app_initApplication(app_ApplicationDesc* applicationDesc) {
    if (app__win32AppCtx->initialized) {
        return;
    }
    app__win32AppCtx->desc = *applicationDesc;
    rx__winCallBeforeDone = app_startApplication;
}

void app_stopApplication (void) {
    // destroy all windows...
}

void* app_getUserData(void) {
    return app__win32AppCtx->desc.user;
}

void app_setUserData(void* userPtr) {
    app__win32AppCtx->desc.user = userPtr;
}

u32 app_maxWindows(void) {
    return OS_MAX_WINDOWS;
}

app_window app_makeWindow(app_WindowDesc* desc) {
    u32 idx = 0;

    for (;app__win32AppCtx->windows[idx].active =! true && idx != countOf(app__win32AppCtx->windows););
    app_window window;

    if (idx == countOf(app__win32AppCtx->windows)) {
        window.id = 0;
        ASSERT(!"No free window!");
        return window;
    }


    app__Win32Window* win = app__win32AppCtx->windows + idx;
    win->active = true;
    win->hWnd = CreateWindowExA(0, app__Win32WindowClass, "REPLACE ME", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, desc->width ? desc->width : 420, desc->height ? desc->height : 320, NULL, NULL, NULL, NULL);

    window.id = idx + 1;
    return window;
}


typedef struct app_WindowContext {
    struct {
        u32 (*getFrameBuffer)(void*);
        void* userPtr;
    } gl;
} app_WindowContext;

void app_showWindow(app_window window) {
    ASSERT(window.id);
    window.id -= 1;
    ShowWindow(app__win32AppCtx->windows[window.id].hWnd, SW_SHOWNORMAL);
}

void app_hideWindow(app_window window) {
    ASSERT(window.id);
    window.id -= 1;
    ShowWindow(app__win32AppCtx->windows[window.id].hWnd, SW_HIDE);
}

void app_destroyWindow(app_window window) {
    ASSERT(window.id);
    window.id -= 1;
    app__win32AppCtx->windows[window.id].active = false;
    DestroyWindow(app__win32AppCtx->windows[window.id].hWnd);
}

void* app_getGraphicsHandle(app_window window) {
    ASSERT(window.id);
    window.id -= 1;
    return (void*) &app__win32AppCtx->windows[window.id].hWnd;
}

#else
#error "OS no implemented"
#endif

#endif // os_DlL_HOST
