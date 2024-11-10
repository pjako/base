#include "base/base.h"
#include "base/base_types.h"
#include "base/base_str.h"
#include "base/base_mem.h"
#include "base/base_math.h"

#include "os/os.h"
#include "log/log.h"
#include "app/app.h"

#ifndef APP__ASSERT
#ifndef ASSERT
#include <assert.h>
#define APP__ASSERT(EXPR) assert(EXPR)
#else
#define APP__ASSERT(EXPR) ASSERT(EXPR)
#endif
#endif

#define GFX_BACKEND_OGL 1

#if GFX_BACKEND_OGL == 1 || defined(GFX_BACKEND_OGL)
    #undef GFX_BACKEND_OGL
    #define GFX_BACKEND_OGL 1
#else
    #define GFX_BACKEND_OGL 0
#endif

#if GFX_BACKEND_MTL == 1 || defined(GFX_BACKEND_MTL)
    #undef GFX_BACKEND_MTL
    #define GFX_BACKEND_MTL 1
#else
    #if OS_APPLE && GFX_BACKEND_OGL == 0
        // default to Metal if OGL is not defined
        #define GFX_BACKEND_MTL 1
    #else
        #define GFX_BACKEND_MTL 0
    #endif
#endif

#if GFX_BACKEND_DX12 == 1 || defined(GFX_BACKEND_DX12)
    #undef GFX_BACKEND_DX12
    #define GFX_BACKEND_DX12 1
#else
    #define GFX_BACKEND_DX12 0
#endif

#if GFX_BACKEND_VK == 1 || defined(GFX_BACKEND_VK)
    #undef GFX_BACKEND_VK
    #define GFX_BACKEND_VK 1
#else
    #define GFX_BACKEND_VK 0
#endif


// There seems to be no good way to support windows withs OGL on MacOS with a single callback
// iOS and MacOS have only a single window as well
#if (OS_OSX && GFX_BACKEND_OGL) || OS_IOS || OS_ANDROID
#define OS_MAX_WINDOWS 1
#else
#define OS_MAX_WINDOWS 100
#endif

#if OS_DLL_HOST
S8 app__useDll(void) {
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



LOCAL void app__initEvent(app_appEventType type, app_window window);

#if OS_APPLE
    #ifndef GL_SILENCE_DEPRECATION
        #define GL_SILENCE_DEPRECATION
    #endif

    #if OS_OSX
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
#endif


struct app_AppCtx;

static struct app__AppCtx* app__appCtx;

#if OS_APPLE

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


#if GFX_BACKEND_OGL
    #if OS_IOS
        #define GfxView GLKView
        #else
        #define GfxView NSOpenGLView
    #endif
#else
    #if OS_IOS
        #define GfxView UIView
        #else
        #define GfxView NSView
    #endif
#endif // GFX_BACKEND_OGL

@interface AppView : GfxView

#if OS_OSX && GFX_BACKEND_OGL
@property (strong) NSTimer* timer;
#endif // OS_OSX && GFX_BACKEND_OGL


#if GFX_BACKEND_MTL
-(CALayer*) makeBackingLayer;
@property(nonatomic, strong) CALayer* caLayer;
#endif // GFX_BACKEND_MTL

#if OS_OSX
@property(nonatomic, strong) NSTrackingArea* trackingArea;
#endif // OS_OSX
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
@property(nonatomic)         bx            highDpi;
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
    bx mainCalled;
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
    S8 dllFileName;
    S8 dllFullPath;
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
LOCAL app__AppleCtx* app__appCtx;
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
    // os_Dl* os_dlOpen(S8 filePath);
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

// API os_Dl* os_dlOpen(S8 filePath);
// API void   os_dlClose(os_Dl* handle);
// API void*  os_DlSym(os_Dl* handle, const char* symbol);

#ifndef OS_NO_ENTRY


typedef i32(app__dllMainFn)(app__AppleCtx* ctx, int argc, char* argv[]);
typedef void(app__dllContinueFn)(app__AppleCtx* ctx);
#ifdef OS_DLLCLIENT
extern i32 app__dllMain(app__AppleCtx* ctx, int argc, char* argv[]) {
    app__isDll = true;
    app__appCtx = ctx;
    i32 val = app_main(argc, argv);
    return val;
}

extern void app__dllContinue(app__AppleCtx* ctx) {
    app__isDll = true;
    app__appCtx = ctx;
    app_continue();
}
#endif

LOCAL bx app__clientDllCloseAndLoad(Arena* tmpArena) {
    if (app__appCtx->clientDll) {
        log_trace(tmpArena, str8("dll close last"));
        os_dlClose(app__appCtx->clientDll);
        app__appCtx->clientDll = NULL;
    }
    bx success = true;
    os_Dl* dllHandle = NULL;
    mem_scoped(tempMem, tmpArena) {
        i32 oldIdx = app__appCtx->dllIdx;
        app__appCtx->dllIdx += 1;
        i32 idx = app__appCtx->dllIdx;
        S8 oldTargetFileName = str_join(tempMem.arena, app__appCtx->dllFullPath, str8("_"), oldIdx, str8(".dylib"));
        S8 targetFileName = str_join(tempMem.arena, app__appCtx->dllFullPath, str8("_"), idx, str8(".dylib"));
        S8 targetFileNameB = str_join(tempMem.arena, app__appCtx->dllFullPath, str8("_"), idx);
        S8 dllFullPath = str_join(tempMem.arena, app__appCtx->dllFullPath, str8(".dylib"));

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
        S8 dllData = os_fileRead(tempMem.arena, dllFullPath);
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
        app__appCtx->clientDll = os_dlOpen(targetFileNameB);
        log_trace(tempMem.arena, str8("dll load:"), targetFileName);
        if (!app__appCtx->clientDll) {
            success = false;
        }
    }
    return success;
}

#ifdef OS_DLLHOST
LOCAL void app__dmonFileWatchCallback(dmon_watch_id watch_id, dmon_action action, const char* rootdir, const char* filepath, const char* oldfilepath, app__AppleCtx* user) {
    if ( action == DMON_ACTION_CREATE || action == DMON_ACTION_MODIFY) {
        S8 filePath = str_fromNullTerminatedCharPtr(filepath);
        if (str_hasSuffix(filePath, str8(".dylib")) && str_isEqual(str_to(filePath, filePath.size - str8(".dylib").size), user->dllFileName)) {
            user->loadNewClientDll = true;
        }
    }

}

LOCAL void app_appleStartApplication(void);
LOCAL i32 app__hostStart(int argc, char* argv[]) {
    S8 dllFile = app__useDll();
    app__appCtx->dllFullPath = dllFile;
    app__appCtx->dllFileName = str_from(dllFile, str_lastIndexOfChar(dllFile, '/') + 1);

    if (!app__clientDllCloseAndLoad(app__appCtx->mainArena)) {
        //app_log(str8("No Dll to load!"));
        return 1;
    }

    os_Dl* dllHandle = app__appCtx->clientDll;
    app__dllMainFn* dllMainFn = os_DlSym(dllHandle, "app__dllMain");
    if (dllMainFn == NULL) {
        // error!
        //app_log(str8("No entry point in client dll! (app__dllMain)"));
        return 1;
    }

    i32 val = dllMainFn(app__appCtx, argc, argv);

    if (val != 0) {
        // some error happened, early exit
        return val;
    }
    // we want to call the host app initializer so we overwrite it for this case
    if (app__appCtx->rx__appleCallBeforeDone) {
        app__appCtx->rx__appleCallBeforeDone = app_appleStartApplication;
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

    if (app__appCtx->rx__appleCallBeforeDone) {
        app__appCtx->rx__appleCallBeforeDone();
    }
    return 0;
}
#endif
void* app_getUserData(void) {
    return app__appCtx->desc.user;
}

void app_setUserData(void* userPtr) {
    app__appCtx->desc.user = userPtr;
}

LOCAL bx app__eventsEnabled(void) {
    /* only send events when an event callback is set, and the init function was called */
    return app__appCtx->desc.event && app__appCtx->initCalled;
}

LOCAL bx app__callEvent(app_AppEvent* e) {
    if (!app__appCtx->cleanupCalled) {
        if (app__appCtx->desc.event) {
            app__appCtx->desc.event(e);
        }
    }
    if (app__appCtx->eventConsumed) {
        app__appCtx->eventConsumed = false;
        return true;
    }
    return false;
}

LOCAL void app__initAndCallEvent(app_appEventType type) {
    if (app__eventsEnabled()) {
        app__initEvent(type);
        app__callEvent(&app__appCtx->event);
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
        return app__appCtx->keyCodes[scanCode];
    }

    return app_keyCode_invalid;
}

LOCAL void app__initKeyTable(void) {
    app__appCtx->keyCodes[0x1D] = app_keyCode_0;
    app__appCtx->keyCodes[0x12] = app_keyCode_1;
    app__appCtx->keyCodes[0x13] = app_keyCode_2;
    app__appCtx->keyCodes[0x14] = app_keyCode_3;
    app__appCtx->keyCodes[0x15] = app_keyCode_4;
    app__appCtx->keyCodes[0x17] = app_keyCode_5;
    app__appCtx->keyCodes[0x16] = app_keyCode_6;
    app__appCtx->keyCodes[0x1A] = app_keyCode_7;
    app__appCtx->keyCodes[0x1C] = app_keyCode_8;
    app__appCtx->keyCodes[0x19] = app_keyCode_9;
    app__appCtx->keyCodes[0x00] = app_keyCode_A;
    app__appCtx->keyCodes[0x0B] = app_keyCode_B;
    app__appCtx->keyCodes[0x08] = app_keyCode_C;
    app__appCtx->keyCodes[0x02] = app_keyCode_D;
    app__appCtx->keyCodes[0x0E] = app_keyCode_E;
    app__appCtx->keyCodes[0x03] = app_keyCode_F;
    app__appCtx->keyCodes[0x05] = app_keyCode_G;
    app__appCtx->keyCodes[0x04] = app_keyCode_H;
    app__appCtx->keyCodes[0x22] = app_keyCode_I;
    app__appCtx->keyCodes[0x26] = app_keyCode_J;
    app__appCtx->keyCodes[0x28] = app_keyCode_K;
    app__appCtx->keyCodes[0x25] = app_keyCode_L;
    app__appCtx->keyCodes[0x2E] = app_keyCode_M;
    app__appCtx->keyCodes[0x2D] = app_keyCode_N;
    app__appCtx->keyCodes[0x1F] = app_keyCode_O;
    app__appCtx->keyCodes[0x23] = app_keyCode_P;
    app__appCtx->keyCodes[0x0C] = app_keyCode_Q;
    app__appCtx->keyCodes[0x0F] = app_keyCode_R;
    app__appCtx->keyCodes[0x01] = app_keyCode_S;
    app__appCtx->keyCodes[0x11] = app_keyCode_T;
    app__appCtx->keyCodes[0x20] = app_keyCode_U;
    app__appCtx->keyCodes[0x09] = app_keyCode_V;
    app__appCtx->keyCodes[0x0D] = app_keyCode_W;
    app__appCtx->keyCodes[0x07] = app_keyCode_X;
    app__appCtx->keyCodes[0x10] = app_keyCode_Y;
    app__appCtx->keyCodes[0x06] = app_keyCode_Z;
    app__appCtx->keyCodes[0x27] = app_keyCode_apostrophe;
    app__appCtx->keyCodes[0x2A] = app_keyCode_backslash;
    app__appCtx->keyCodes[0x2B] = app_keyCode_comma;
    app__appCtx->keyCodes[0x18] = app_keyCode_equal;
    app__appCtx->keyCodes[0x32] = app_keyCode_gravenAccent;
    app__appCtx->keyCodes[0x21] = app_keyCode_leftBracket;
    app__appCtx->keyCodes[0x1B] = app_keyCode_minus;
    app__appCtx->keyCodes[0x2F] = app_keyCode_period;
    app__appCtx->keyCodes[0x1E] = app_keyCode_rightBracket;
    app__appCtx->keyCodes[0x29] = app_keyCode_semicolon;
    app__appCtx->keyCodes[0x2C] = app_keyCode_slash;
    app__appCtx->keyCodes[0x0A] = app_keyCode_world1;
    app__appCtx->keyCodes[0x33] = app_keyCode_backspace;
    app__appCtx->keyCodes[0x39] = app_keyCode_capsLock;
    app__appCtx->keyCodes[0x75] = app_keyCode_delete;
    app__appCtx->keyCodes[0x7D] = app_keyCode_down;
    app__appCtx->keyCodes[0x77] = app_keyCode_end;
    app__appCtx->keyCodes[0x24] = app_keyCode_enter;
    app__appCtx->keyCodes[0x35] = app_keyCode_escape;
    app__appCtx->keyCodes[0x7A] = app_keyCode_F1;
    app__appCtx->keyCodes[0x78] = app_keyCode_F2;
    app__appCtx->keyCodes[0x63] = app_keyCode_F3;
    app__appCtx->keyCodes[0x76] = app_keyCode_F4;
    app__appCtx->keyCodes[0x60] = app_keyCode_F5;
    app__appCtx->keyCodes[0x61] = app_keyCode_F6;
    app__appCtx->keyCodes[0x62] = app_keyCode_F7;
    app__appCtx->keyCodes[0x64] = app_keyCode_F8;
    app__appCtx->keyCodes[0x65] = app_keyCode_F9;
    app__appCtx->keyCodes[0x6D] = app_keyCode_F10;
    app__appCtx->keyCodes[0x67] = app_keyCode_F11;
    app__appCtx->keyCodes[0x6F] = app_keyCode_F12;
    app__appCtx->keyCodes[0x69] = app_keyCode_F13;
    app__appCtx->keyCodes[0x6B] = app_keyCode_F14;
    app__appCtx->keyCodes[0x71] = app_keyCode_F15;
    app__appCtx->keyCodes[0x6A] = app_keyCode_F16;
    app__appCtx->keyCodes[0x40] = app_keyCode_F17;
    app__appCtx->keyCodes[0x4F] = app_keyCode_F18;
    app__appCtx->keyCodes[0x50] = app_keyCode_F19;
    app__appCtx->keyCodes[0x5A] = app_keyCode_F20;
    app__appCtx->keyCodes[0x73] = app_keyCode_home;
    app__appCtx->keyCodes[0x72] = app_keyCode_insert;
    app__appCtx->keyCodes[0x7B] = app_keyCode_left;
    app__appCtx->keyCodes[0x3A] = app_keyCode_leftAlt;
    app__appCtx->keyCodes[0x3B] = app_keyCode_leftControl;
    app__appCtx->keyCodes[0x38] = app_keyCode_leftShift;
    app__appCtx->keyCodes[0x37] = app_keyCode_leftSuper;
    app__appCtx->keyCodes[0x6E] = app_keyCode_menu;
    app__appCtx->keyCodes[0x47] = app_keyCode_numLock;
    app__appCtx->keyCodes[0x79] = app_keyCode_pageDown;
    app__appCtx->keyCodes[0x74] = app_keyCode_pageUp;
    app__appCtx->keyCodes[0x7C] = app_keyCode_right;
    app__appCtx->keyCodes[0x3D] = app_keyCode_rightAlt;
    app__appCtx->keyCodes[0x3E] = app_keyCode_rightControl;
    app__appCtx->keyCodes[0x3C] = app_keyCode_rightShift;
    app__appCtx->keyCodes[0x36] = app_keyCode_rightSuper;
    app__appCtx->keyCodes[0x31] = app_keyCode_space;
    app__appCtx->keyCodes[0x30] = app_keyCode_tab;
    app__appCtx->keyCodes[0x7E] = app_keyCode_up;
    app__appCtx->keyCodes[0x52] = app_keyCode_keypad0;
    app__appCtx->keyCodes[0x53] = app_keyCode_keypad1;
    app__appCtx->keyCodes[0x54] = app_keyCode_keypad2;
    app__appCtx->keyCodes[0x55] = app_keyCode_keypad3;
    app__appCtx->keyCodes[0x56] = app_keyCode_keypad4;
    app__appCtx->keyCodes[0x57] = app_keyCode_keypad5;
    app__appCtx->keyCodes[0x58] = app_keyCode_keypad6;
    app__appCtx->keyCodes[0x59] = app_keyCode_keypad7;
    app__appCtx->keyCodes[0x5B] = app_keyCode_keypad8;
    app__appCtx->keyCodes[0x5C] = app_keyCode_keypad9;
    app__appCtx->keyCodes[0x45] = app_keyCode_keypadAdd;
    app__appCtx->keyCodes[0x41] = app_keyCode_keypadDecimal;
    app__appCtx->keyCodes[0x4B] = app_keyCode_keypadDivide;
    app__appCtx->keyCodes[0x4C] = app_keyCode_keypadEnter;
    app__appCtx->keyCodes[0x51] = app_keyCode_keypadEqual;
    app__appCtx->keyCodes[0x43] = app_keyCode_keypadMultiply;
    app__appCtx->keyCodes[0x4E] = app_keyCode_keypadSubtract;
}


///////////////////
// Input

#pragma mark Mouse

void app_lockMouse(bx lock) {
    if (lock == app__appCtx->mouse.locked) {
        return;
    }
    app__appCtx->mouse.delta.x = 0.0f;
    app__appCtx->mouse.delta.y = 0.0f;
    app__appCtx->mouse.locked = lock;
    /*
        NOTE that this code doesn't warp the mouse cursor to the window
        center as everybody else does it. This lead to a spike in the
        *second* mouse-moved event after the warp happened. The
        mouse centering doesn't seem to be required (mouse-moved events
        are reported correctly even when the cursor is at an edge of the screen).
        NOTE also that the hide/show of the mouse cursor should properly
        stack with calls to sapp_show_mouse()
    */
    if (app__appCtx->mouse.locked) {
        CGAssociateMouseAndMouseCursorPosition(NO);
        [NSCursor hide];
    } else {
        [NSCursor unhide];
        CGAssociateMouseAndMouseCursorPosition(YES);
    }
}

bx app_isMouseLocked(void) {
    return app__appCtx->mouse.locked;
}

LOCAL void app__mouseEvents(app_appEventType type, app_mouseButton btn, u32 mod) {
    if (app__eventsEnabled()) {
        app__initEvent(type);
        app__appCtx->event.mouse.button = btn;
        app__appCtx->event.modifiers = mod;
        app__callEvent(&app__appCtx->event);
    }
}

LOCAL void app__updateMouse(NSEvent* event) {
    if (!app__appCtx->mouse.locked) {
        AppWindow* window = (AppWindow*) event.window;
        const NSPoint mousePos = event.locationInWindow;
        f32 dpiScale = window.dpiScale;
        f32 newX = mousePos.x * dpiScale;
        f32 newY = app__appCtx->frameBufferHeight - (mousePos.y * dpiScale) - 1;
        //log_trace(str8("native mouse x"), newX, str8(" y: "), newY);
        /* don't update dx/dy in the very first update */
        if (app__appCtx->mouse.posValid) {
            app__appCtx->mouse.delta.x = newX - app__appCtx->mouse.pos.x;
            app__appCtx->mouse.delta.y = newY - app__appCtx->mouse.pos.y;
        }
        app__appCtx->mouse.pos.x = newX;
        app__appCtx->mouse.pos.y = newY;
        app__appCtx->mouse.posValid = true;
    }
}

#pragma mark Mouse

LOCAL void app__keyEvent(app_appEventType type, app_keyCode key, bx repeat, u32 mod) {
    if (app__eventsEnabled()) {
        app__initEvent(type);
        app__appCtx->event.keyCode = key;
        app__appCtx->event.keyRepeat = repeat;
        app__appCtx->event.modifiers = mod;
        app__callEvent(&app__appCtx->event);
    }
}

#pragma mark Window

/* NOTE: unlike the iOS version of this function, the macOS version
    can dynamically update the DPI scaling factor when a window is moved
    between HighDPI / LowDPI screens.
*/
LOCAL void app__updateDimensions(AppWindow* appleWindow) {
    if (appleWindow.highDpi) {
        appleWindow.dpiScale = [appleWindow screen].backingScaleFactor;
    } else {
        appleWindow.dpiScale = 1.0f;
    }

    const NSRect bounds = [appleWindow.gfxView bounds];


    appleWindow.windowWidth = bounds.size.width; //(i32) f32_round(bounds.size.width);
    appleWindow.windowHeight = bounds.size.height; //(i32) f32_round(bounds.size.height);

    appleWindow.frameBufferWidth = (int)roundf(bounds.size.width * appleWindow.dpiScale);
    appleWindow.frameBufferHeight = (int)roundf(bounds.size.height * appleWindow.dpiScale);

    app__appCtx->frameBufferWidth = appleWindow.frameBufferWidth;
    app__appCtx->frameBufferHeight = appleWindow.frameBufferHeight;
    app__appCtx->windowWidth = appleWindow.windowWidth;
    app__appCtx->windowHeight = appleWindow.windowHeight;


    const CGSize fb_size = appleWindow.gfxView.frame.size;
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
        //appleWindow.gfxView.frame = frame;
        if (!appleWindow.firstFrame) {
            app__appCtx->event.window.id = appleWindow.windowId;
            app__appCtx->frameBufferWidth = appleWindow.frameBufferWidth;
            app__appCtx->frameBufferHeight = appleWindow.frameBufferHeight;
            app__appCtx->windowWidth = appleWindow.windowWidth;
            app__appCtx->windowHeight = appleWindow.windowHeight;
            app__initEvent(app_appEventType_resized);
            app__callEvent(&app__appCtx->event);
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

@implementation AppWindow

- (void)windowDidResize:(NSNotification*)notification {
    unusedVars(notification);
    app__updateDimensions(self);
}

- (void)windowDidChangeScreen:(NSNotification*)notification {
    unusedVars(notification);
    //_sapp_timing_reset(&_sapp.timing);
    app__updateDimensions(self);
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    unusedVars(notification);
    app__initEvent(app_appEventType_iconified);
    app__appCtx->event.window.id = self.windowId;
    app__callEvent(&app__appCtx->event);
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    unusedVars(notification);
    app__initEvent(app_appEventType_restored);
    app__appCtx->event.window.id = self.windowId;
    app__callEvent(&app__appCtx->event);
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    unusedVars(notification);
    app__appCtx->focusedWindow.id = self.windowId;
    app__initEvent(app_appEventType_focused);
    app__callEvent(&app__appCtx->event);
}

- (void)windowDidResignKey:(NSNotification*)notification {
    unusedVars(notification);
    if (app__appCtx->focusedWindow.id == self.windowId) {
        app__appCtx->focusedWindow.id = 0;
    }
    app__initEvent(app_appEventType_unfocused);
    app__appCtx->event.window.id = self.windowId;
    app__callEvent(&app__appCtx->event);
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


@implementation AppView

#if OS_OSX && GFX_BACKEND_OGL
- (id)init {
    NSOpenGLPixelFormatAttribute attrs[32];
    int i = 0;
    attrs[i++] = NSOpenGLPFAAccelerated;
    attrs[i++] = NSOpenGLPFADoubleBuffer;
    attrs[i++] = NSOpenGLPFAOpenGLProfile;
    const int glVersion = 41; //_sapp.desc.gl_major_version * 10 + _sapp.desc.gl_minor_version;
    //switch(glVersion) {
    //    case 10: attrs[i++] = NSOpenGLProfileVersionLegacy;  break;
    //    case 32: attrs[i++] = NSOpenGLProfileVersion3_2Core; break;
    //    case 41: attrs[i++] = NSOpenGLProfileVersion4_1Core; break;
    //    default: ASSERT(!"MACOS_INVALID_NSOPENGL_PROFILE");//_SAPP_PANIC(MACOS_INVALID_NSOPENGL_PROFILE);
    //}
    attrs[i++] = NSOpenGLProfileVersion4_1Core;

    attrs[i++] = NSOpenGLPFAColorSize; attrs[i++] = 24;
    attrs[i++] = NSOpenGLPFAAlphaSize; attrs[i++] = 8;
    attrs[i++] = NSOpenGLPFADepthSize; attrs[i++] = 24;
    attrs[i++] = NSOpenGLPFAStencilSize; attrs[i++] = 8;

    //if (desc->sampleCount > 1) {
    //    attrs[i++] = NSOpenGLPFAMultisample;
    //    attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 1;
    //    attrs[i++] = NSOpenGLPFASamples; attrs[i++] = (NSOpenGLPixelFormatAttribute) desc->sampleCount;
    //}
    //else {
        attrs[i++] = NSOpenGLPFASampleBuffers; attrs[i++] = 0;
    //}
    attrs[i++] = 0;
    NSOpenGLPixelFormat* glpixelformatObj = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    ASSERT(glpixelformatObj != nil);

    self = [super initWithFrame:NSMakeRect(0, 0, 400, 600) pixelFormat:glpixelformatObj];
    OS_OBJC_RELESE(glpixelformatObj);

#if GFX_BACKEND_MTL
    [self setLayer:[window.gfxView makeBackingLayer]];
    [self setWantsLayer:YES];
    [self.layer setContentsScale:[window backingScaleFactor]];
#endif
    [self prepareOpenGL];
    [self.openGLContext makeCurrentContext];
    
    if (self) {
        NSTimer* timerObj = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(timerFired:) userInfo:nil repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:timerObj forMode:NSDefaultRunLoopMode];
        self.timer = timerObj;
    }
    return self;
}

- (void)timerFired:(id)sender {
    [self setNeedsDisplay:YES];
}

#endif // OS_OSX && GFX_BACKEND_OGL

- (BOOL) isOpaque {
    return YES;
}

- (BOOL)canBecomeKey {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}


#if GFX_BACKEND_OGL
    #if OS_IOS
- (void)drawRect:(CGRect)rect {
    ASSERT(app__appCtx);
    ASSERT(app__appCtx->desc.update);
    if (!app__appCtx->mainCalled) {
        return;
    }
    @autoreleasepool {
        app__appCtx->desc.update();
    }
}
    #else
- (void)drawRect:(NSRect)rect {
    [self.openGLContext makeCurrentContext];
    ASSERT(app__appCtx);
    ASSERT(app__appCtx->desc.update);
    if (!app__appCtx->mainCalled) {
        return;
    }
    @autoreleasepool {
        app__appCtx->desc.update();
    }
    [self.openGLContext flushBuffer];
}
    #endif // OS_IOS
#endif // GFX_BACKEND_OGL
#if OS_IOS
#else
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
    if (0 == app__appCtx->mouse.buttons) {
        app__mouseEvents(app_appEventType_mouseEnter, app_mouseButton__invalid, app__inputMods(event));
    }
}

- (void)mouseExited:(NSEvent*)event {
    app__updateMouse(event);
    if (0 == app__appCtx->mouse.buttons) {
        app__mouseEvents(app_appEventType_mouseLeave, app_mouseButton__invalid, app__inputMods(event));
    }
}

- (void)mouseDown:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseDown, app_mouseButton_left, app__inputMods(event));
    app__appCtx->mouse.buttons |= (1<<app_mouseButton_left);
}

- (void)mouseUp:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseUp, app_mouseButton_left, app__inputMods(event));
    app__appCtx->mouse.buttons &= ~(1<<app_mouseButton_left);
}

- (void)rightMouseDown:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseDown, app_mouseButton_right, app__inputMods(event));
    app__appCtx->mouse.buttons |= (1<<app_mouseButton_right);
}

- (void)rightMouseUp:(NSEvent*)event {
    app__updateMouse(event);
    app__mouseEvents(app_appEventType_mouseUp, app_mouseButton_right, app__inputMods(event));
    app__appCtx->mouse.buttons &= ~(1<<app_mouseButton_right);
}

- (void)otherMouseDown:(NSEvent*)event {
    app__updateMouse(event);
    if (2 == event.buttonNumber) {
        app__mouseEvents(app_appEventType_mouseDown, app_mouseButton_middle, app__inputMods(event));
        app__appCtx->mouse.buttons |= (1<<app_mouseButton_middle);
    }
}

- (void)otherMouseUp:(NSEvent*)event {
    app__updateMouse(event);
    if (2 == event.buttonNumber) {
        app__mouseEvents(app_appEventType_mouseUp, app_mouseButton_middle, app__inputMods(event));
        app__appCtx->mouse.buttons &= (1<<app_mouseButton_middle);
    }
}

- (void)otherMouseDragged:(NSEvent*)event {
    app__updateMouse(event);
    if (2 == event.buttonNumber) {
        if (app__appCtx->mouse.locked) {
            app__appCtx->mouse.delta.x = [event deltaX];
            app__appCtx->mouse.delta.y = [event deltaY];
        }
        app__mouseEvents(app_appEventType_mouseMove, app_mouseButton__invalid, app__inputMods(event));
    }
}
- (void)mouseMoved:(NSEvent*)event {
    app__updateMouse(event);
    if (app__appCtx->mouse.locked) {
        app__appCtx->mouse.delta.x = [event deltaX];
        app__appCtx->mouse.delta.y = [event deltaY];
    }
    app__mouseEvents(app_appEventType_mouseMove, app_mouseButton__invalid, app__inputMods(event));
}

- (void)mouseDragged:(NSEvent*)event {
    app__updateMouse(event);
    if (app__appCtx->mouse.locked) {
        app__appCtx->mouse.delta.x = [event deltaX];
        app__appCtx->mouse.delta.y = [event deltaY];
    }
    app__mouseEvents(app_appEventType_mouseMove, app_mouseButton__invalid, app__inputMods(event));
}

- (void)rightMouseDragged:(NSEvent*)event {
    app__updateMouse(event);
    if (app__appCtx->mouse.locked) {
        app__appCtx->mouse.delta.x = [event deltaX];
        app__appCtx->mouse.delta.y = [event deltaY];
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
            app__appCtx->event.modifiers = app__inputMods(event);
            app__appCtx->event.mouse.scroll.x = dx;
            app__appCtx->event.mouse.scroll.y = dy;
            app__callEvent(&app__appCtx->event);
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
            app__appCtx->event.modifiers = mods;
            for (NSUInteger i = 0; i < len; i++) {
                const unichar codepoint = [chars characterAtIndex:i];
                if ((codepoint & 0xFF00) == 0xF700) {
                    continue;
                }
                app__appCtx->event.charCode = codepoint;
                app__appCtx->event.keyRepeat = event.isARepeat;
                app__callEvent(&app__appCtx->event);
            }
        }
        /* if this is a Cmd+V (paste), also send a CLIPBOARD_PASTE event */
        #if 0
        if (app__appCtx->clipboard.enabled && (mods == app_inputModifier_super) && (key_code == app_keyCode_V)) {
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
    const uint32_t old_f = app__appCtx->flagsChangedStore;
    const uint32_t new_f = event.modifierFlags;
    app__appCtx->flagsChangedStore = new_f;
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
#endif
@end

#if OS_IOS
#else
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
    if (app__appCtx->desc.init) {
        //mem_Scratch scratch = mem_scratchStart(app_tempMemory());
        app__appCtx->desc.init();
        //mem_scratchEnd(&scratch);
    }


#ifdef OS_DLLHOST
    dmon_init();
    mem_scoped(tempMemory, app__appCtx->mainArena) {
        i64 lastIdx = str_lastIndexOfChar(app__appCtx->dllFullPath, '/');
        S8 dllFolder = str_to(app__appCtx->dllFullPath, lastIdx + 1);
        S8 dllCharFolder = str_join(tempMemory.arena, dllFolder, '\0');
        dmon_watch((unsigned char*) dllCharFolder.content, app__dmonFileWatchCallback, 0, app__appCtx);
    }
#endif

    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    [NSApp activateIgnoringOtherApps:YES];
    [NSEvent setMouseCoalescingEnabled:NO];
    
    app_appInitThread();

#if GFX_BACKEND_MTL
    CVDisplayLinkCreateWithActiveCGDisplays(&app__appCtx->displayLink);
    CVDisplayLinkSetOutputCallback(app__appCtx->displayLink, &app__appleDisplayLinkCallback, nil);
    CVDisplayLinkStart(app__appCtx->displayLink);
#endif

#if OS_OSX
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillResignActive:) name:NSApplicationWillResignActiveNotification object:nil];
    //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:NSApplicationDidBecomeActive object:nil];
#endif
    //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];

    app__appCtx->mainCalled = true;
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
#else
-(void)appWillResignActive:(NSNotification*)note {
#endif
    if (!app__appCtx->suspended) {
        app__appCtx->suspended = true;
        app__initAndCallEvent(app_appEventType_suspended);
    }
}

#if OS_IOS
- (void)applicationDidBecomeActive:(UIApplication *)application {
#elif OS_OSX
-(void) applicationDidBecomeActive:(NSNotification *)notification {
#endif
    if (app__appCtx->suspended) {
        app__appCtx->suspended = false;
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
        if (app__appCtx->loadNewClientDll) {
            app__appCtx->loadNewClientDll = false;
            app__appCtx->desc.prepareReloadDll();
            if (app__clientDllCloseAndLoad(app__appCtx->mainArena)) {

                app__dllContinueFn* dllMainFn = os_dlSym(app__appCtx->clientDll, "app__dllContinue");
                if (dllMainFn == NULL) {
                    // error!
                    // app_log(str8("No entry point in client dll! (app__dllMain)"));
                    return 1;
                }

                dllMainFn(app__appCtx);
            }
        }
        //dispatch_async(dispatch_get_main_queue(), ^{
        app__appCtx->desc.update();
        //});
        
    }
    app_appNextFrameThread();
    app__appCtx->frameCount++;
    return kCVReturnSuccess;
}

static void app__appleSetWindowName(AppWindow* window, S8 name) {
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
    window.highDpi = desc->dpi == app_windowDpi_low ? false : true; 
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

    window.gfxView = [[AppView alloc] init];
    window.contentView = window.gfxView;

    [window layoutIfNeeded];
    window.gfxView.frame = NSMakeRect(0, 0, desc->width, desc->height);
    
    [window makeFirstResponder:window.contentView];
    app__appleSetWindowName(window, desc->title);
    [window makeKeyAndOrderFront:nil];
    [window layoutIfNeeded];
    [window display];
    [app__appCtx->windows addObject:window];
    app__updateDimensions(window);


    return [app__appCtx->windows count] - 1;
}

void app_appleStartApplication(void) {
    app__initKeyTable();
    [NSApplication sharedApplication];
    OsAppDelegate* delegate = [[OsAppDelegate alloc] init];
    app__appCtx->delegate = delegate;
    NSApp.delegate = delegate;
    [NSApp run];
}

void app_initApplication(app_ApplicationDesc* applicationDesc) {
    if (app__appCtx != NULL) {
        return;
    }
    BaseMemory baseMem = os_getBaseMemory();
    mms appMemoryBudget = applicationDesc->appMemoryBudget == 0 ? MEGABYTE(20) : applicationDesc->appMemoryBudget;
    Arena* mainArena = mem_makeArena(&baseMem, appMemoryBudget);
    app__appCtx = mem_arenaPushStructZero(mainArena, app__AppleCtx);
    app__appCtx->mainArena = mainArena;


    app__appCtx->started = true;
    void* user = app__appCtx->desc.user;
    app__appCtx->desc = *applicationDesc;
    if (!app__appCtx->desc.user) {
        app__appCtx->desc.user = user;
    }
    app__appCtx->windows = [NSMutableArray array];
#ifndef OS_NO_ENTRY 
    app__appCtx->rx__appleCallBeforeDone = app_appleStartApplication;
#endif
    app__appCtx->initCalled = true;
}

void app_stopApplication (void) {
    ASSERT(!"Implement me!");
}

u32 app_maxWindows(void) {
    return OS_MAX_WINDOWS;
}

Vec2 app_getWindowSizeF32(app_window window) {
    AppWindow* osWindow = app__appCtx->windows[window.id - 1];
    Vec2 size;
    size.x = osWindow.contentView.frame.size.width; //osWindow.frameBufferWidth;
    size.y = osWindow.contentView.frame.size.height;

    return size;
}

Vec2i app_getWindowSize(app_window window) {
    Vec2 fSize = app_getWindowSizeF32(window);
    Vec2i size;
    size.x = (i32) f32_round(fSize.x);
    size.y = (i32) f32_round(fSize.y);

    return size;
}

Vec2 app_getWindowFrameBufferSizeF32(app_window window) {
    AppWindow* osWindow = app__appCtx->windows[window.id - 1];
    Vec2 size;
    size.x = osWindow.frameBufferWidth;
    size.y = osWindow.frameBufferHeight;

    return size;
}

app_window app_makeWindow(app_WindowDesc* desc) {
    app_window window;
    window.id = 0;
    if ([app__appCtx->windows count] >= app_maxWindows()) {
        return window;
    }

    window.id = app__appleCreateWindow(desc) + 1;
    return window;
}

void app_showWindow(app_window window) {
    ASSERT(window.id);
    u32 windowIndex = window.id - 1;
    AppWindow* nsWindow = app__appCtx->windows[windowIndex];

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
    AppWindow* nsWindow = app__appCtx->windows[windowIndex];
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
    AppWindow* nsWindow = app__appCtx->windows[windowIndex];
    ASSERT(nsWindow);
    app_hideWindow(window);
    app__appCtx->windows[windowIndex] = (AppWindow*) [NSNull null];
#else
    unusedVars(&window);
#endif // OS_OSX
}

void* app_getGraphicsHandle(app_window window) {
    ASSERT(window.id);
    u32 windowIndex = window.id - 1;
    AppWindow* nsWindow = app__appCtx->windows[windowIndex];
#if GFX_BACKEND_OPENGL
    return NULL;
#else
    return (void*) [nsWindow.gfxView makeBackingLayer];
#endif
}

Arena* app_frameArena(void) {
    return app__appThreadCtx.arenas[(app__appCtx->frameCount % 2)];
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
    mem_arenaPopTo(app__appThreadCtx.arenas[(app__appCtx->frameCount % 2)], 0);
}

void app_appCleanupThread(void) {
    for (u32 idx = 0; idx < countOf(app__appThreadCtx.arenas); idx++) {
        mem_destroyArena(app__appThreadCtx.arenas[idx]);
        app__appThreadCtx.arenas[idx] = NULL;
    }
}
#elif OS_WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

typedef struct app__Window {
    HWND hWnd;
    HMONITOR hmonitor;
    HDC dc;
    app_window window;
    bool active;
    bool fullscreen;
    bool iconified;
    bool mouseLocked;
    struct {
        float x, y;
        float dx, dy;
        bool shown;
        bool locked;
        bool posValid;
        bool mouseTracked;
    } mouse;
    struct {
        bool aware;
        float contentScale;
        float windowScale;
        float mouseScale;
        float scale;
    } dpi;
    float height;
    float width;
    float frameBufferWidth;
    float frameBufferHeight;
    bool rawInputMouseposValid;
    LONG rawInputMouseposX;
    LONG rawInputMouseposY;
    uint8_t mouseCaptureMask;
    uint8_t rawInputData[256];
} app__Window;

typedef struct app__AppCtx {
    bool initialized;
    bool fullscreen;
    bool eventConsumed;
    bool cleanupCalled;
    Arena* mainArena;
    WNDCLASSEXA windowClass;
    app_ApplicationDesc desc;
    struct {
        bool enabled;
    } clipboard;
    app_AppEvent event;
    app__Window windows[OS_MAX_WINDOWS];
    app_keyCode keycodes[app_keyCodes__count];
} app__AppCtx;

static void (*app__winCallBeforeDone)(void);

LOCAL char** app__cmdToUtf8Argv(Arena* arena, LPWSTR w_command_line, i32* o_argc) {
    i32 argc = 0;
    char** argv = 0;
    char* args;

    LPWSTR* w_argv = CommandLineToArgvW(w_command_line, &argc);
    if (w_argv == NULL) {
        ASSERT(!"Win32: failed to parse command line");
    } else {
        mms size = wcslen(w_command_line) * 4;
        u64 byteSize = ((size_t)argc + 1) * sizeof(char*) + size;
        argv = (char**) mem_arenaPush(arena, byteSize);
        mem_setZero(argv, byteSize);
        //argv = (char**) calloc(1, ((size_t)argc + 1) * sizeof(char*) + size);
        ASSERT(argv);
        args = (char*) &argv[argc + 1];
        i32 n;
        for (i32 i = 0; i < argc; ++i) {
            n = WideCharToMultiByte(CP_UTF8, 0, w_argv[i], -1, args, (i32) size, NULL, NULL);
            if (n == 0) {
                ASSERT(!"Win32: failed to convert all arguments to utf8");
                break;
            }
            argv[i] = args;
            size -= (mms) n;
            args += n;
        }
        LocalFree(w_argv);
    }
    *o_argc = argc;
    return argv;
}

int app__main(int argc, char* argv[]) {
    i32 val = app_main(argc, argv);
    if (val != 0) {
        return val;
    }
    if (app__winCallBeforeDone) {
        app__winCallBeforeDone();
    }
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
    int argc_utf8 = 0;
    BaseMemory baseMem = os_getBaseMemory();
    Arena* arena = mem_makeArena(&baseMem, MEGABYTE(1));
    char** argv_utf8 = app__cmdToUtf8Argv(arena, GetCommandLineW(), &argc_utf8);
    i32 retVal = app__main(argc_utf8, argv_utf8);
    mem_destroyArena(arena);

    if (retVal != 0) {
        return retVal;
    }

    if (app__winCallBeforeDone) {
        app__winCallBeforeDone();
    }

    return retVal;
}


#ifdef OS_RELOAD
__declspec(dllexport) app__dllEntry(app__AppCtx* ctx) {
    app__appCtx = ctx;
}
#endif

static bool app__eventsEnabled(void) {
    /* only send events when an event callback is set, and the init function was called */
    return (app__appCtx->desc.event) && app__appCtx->initialized;
}

static uint32_t app__win32Mods(void) {
    uint32_t mods = 0;
    if (GetKeyState(VK_SHIFT) & (1<<15)) {
        mods |= app_inputModifier_shift;
    }
    if (GetKeyState(VK_CONTROL) & (1<<15)) {
        mods |= app_inputModifier_ctrl;
    }
    if (GetKeyState(VK_MENU) & (1<<15)) {
        mods |= app_inputModifier_alt;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & (1<<15)) {
        mods |= app_inputModifier_super;
    }
    const bool swapped = (TRUE == GetSystemMetrics(SM_SWAPBUTTON));
    if (GetAsyncKeyState(VK_LBUTTON)) {
        mods |= swapped ? app_inputModifier_rightMuseButton : app_inputModifier_leftMouseButton;
    }
    if (GetAsyncKeyState(VK_RBUTTON)) {
        mods |= swapped ? app_inputModifier_leftMouseButton : app_inputModifier_rightMuseButton;
    }
    if (GetAsyncKeyState(VK_MBUTTON)) {
        mods |= app_inputModifier_middleMouseButton;
    }
    return mods;
}

static void app__win32WindowEvent(app_appEventType event, app__Window* window) {

}

static void app__win32LockMouse(bool lock) {

}

static void app__win32MouseUpdate(app__Window* window, LPARAM lParam) {
    if (!window->mouseLocked) {
        const float new_x  = (float)GET_X_LPARAM(lParam) * window->dpi.mouseScale;
        const float new_y = (float)GET_Y_LPARAM(lParam) * window->dpi.mouseScale;
        if (window->mouse.posValid) {
            // don't update dx/dy in the very first event
            window->mouse.dx = new_x - window->mouse.x;
            window->mouse.dy = new_y - window->mouse.y;
        }
        window->mouse.x = new_x;
        window->mouse.y = new_y;
        window->mouse.posValid = true;
    }
}

static bool app__callEvent(const app_AppEvent* e) {
    if (!app__appCtx->cleanupCalled) {
        if (app__appCtx->desc.event) {
            app__appCtx->desc.event(e);
        }
    }
    if (app__appCtx->eventConsumed) {
        app__appCtx->eventConsumed = false;
        return true;
    }
    else {
        return false;
    }
}

static void app__win32MouseEvent(app__Window* windows, app_appEventType type, app_mouseButton btn) {
    if (app__eventsEnabled()) {
        app__initEvent(type, windows->window);
        app__appCtx->event.modifiers = app__win32Mods();
        app__appCtx->event.mouse.button = btn;
        app__callEvent(&app__appCtx->event);
    }
}

static void app__win32CaptureMouse(app__Window* windows, uint8_t btn_mask) {
    if (windows->mouseCaptureMask == 0) {
        SetCapture(windows->hWnd);
    }
    windows->mouseCaptureMask |= btn_mask;
}

static void app__win32ReleaseMouse(app__Window* windows, uint8_t btn_mask) {
    if (windows->mouseCaptureMask != 0) {
        windows->mouseCaptureMask &= ~btn_mask;
        if (windows->mouseCaptureMask == 0) {
            ReleaseCapture();
        }
    }
}

static void app__win32ScrollEvent(app__Window* windows, float x, float y) {
    if (app__eventsEnabled()) {
        app__initEvent(app_appEventType_mouseScroll, windows->window);
        app__appCtx->event.modifiers = app__win32Mods();
        app__appCtx->event.mouse.scroll.x = -x / 30.0f;
        app__appCtx->event.mouse.scroll.y = y / 30.0f;
        app__callEvent(&app__appCtx->event);
    }
}

static void app__win32KeyEvent(app__Window* windows, app_appEventType type, int vk, bool repeat) {
    if (app__eventsEnabled() && (vk < app_keyCodes__count)) {
        app__initEvent(type, windows->window);
        app__appCtx->event.modifiers = app__win32Mods();
        app__appCtx->event.keyCode = app__appCtx->keycodes[vk];
        app__appCtx->event.keyRepeat = repeat;
        app__callEvent(&app__appCtx->event);
        /* check if a CLIPBOARD_PASTED event must be sent too */
        if (app__appCtx->clipboard.enabled &&
            (type == app_appEventType_keyDown) &&
            (app__appCtx->event.modifiers == app_inputModifier_ctrl) &&
            (app__appCtx->event.keyCode == app_keyCode_V))
        {
            app__initEvent(app_appEventType_clipboardPasted, windows->window);
            app__callEvent(&app__appCtx->event);
        }
    }
}

static void app__win32CharEvent(app__Window* windows, uint32_t c, bool repeat) {
    if (app__eventsEnabled() && (c >= 32)) {
        app__initEvent(app_appEventType_char, windows->window);
        app__appCtx->event.modifiers = app__win32Mods();
        app__appCtx->event.charCode = c;
        app__appCtx->event.keyRepeat = repeat;
        app__callEvent(&app__appCtx->event);
    }
}

static void app__win32DpiChanged(app__Window* window, LPRECT proposedWinRect) {
    HWND hWnd = window->hWnd;
    /* called on WM_DPICHANGED, which will only be sent to the application
        if sapp_desc.high_dpi is true and the Windows version is recent enough
        to support DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    */
    //SOKOL_ASSERT(_sapp.desc.high_dpi);
    HINSTANCE user32 = LoadLibraryA("user32.dll");
    if (!user32) {
        return;
    }
    typedef UINT(WINAPI * GETDPIFORWINDOW_T)(HWND hwnd);
    GETDPIFORWINDOW_T fn_getdpiforwindow = (GETDPIFORWINDOW_T)(void*)GetProcAddress(user32, "GetDpiForWindow");
    if (fn_getdpiforwindow) {
        UINT dpix = fn_getdpiforwindow(hWnd);
        // NOTE: for high-dpi apps, mouse_scale remains one
        window->dpi.windowScale = (float)dpix / 96.0f;
        window->dpi.contentScale = window->dpi.windowScale;
        window->dpi.scale = window->dpi.windowScale;
        SetWindowPos(hWnd, 0,
            proposedWinRect->left,
            proposedWinRect->top,
            proposedWinRect->right - proposedWinRect->left,
            proposedWinRect->bottom - proposedWinRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    FreeLibrary(user32);
}

static LRESULT os__win32DefaultWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    // find windows this pro belongs to

    
    app__Window* window = NULL; // = app__appCtx->windows + idx;

    for (uint32_t idx = 0; idx < countOf(app__appCtx->windows); idx++) {
        if (app__appCtx->windows[idx].hWnd == hWnd) {
            window = &app__appCtx->windows[idx];
            break;
        }
    }
    if (window == NULL) {
        return;
    }

    APP__ASSERT(window && "Unknown window");

    //app__initEvent(app_appEventType_mouseScroll);
    switch (msg) {
        case WM_CLOSE: {
            //PostQuitMessage(0);
            return 0;
        } break;
        case WM_SYSCOMMAND: {
            switch (wParam & 0xFFF0) {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                    if (window->fullscreen) {
                        /* disable screen saver and blanking in fullscreen mode */
                        return 0;
                    }
                    break;
                case SC_KEYMENU:
                    /* user trying to access menu via ALT */
                    return 0;
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE: {
            const bool iconified = wParam == SIZE_MINIMIZED;
            if (iconified != window->iconified) {
                window->iconified = iconified;
                if (iconified) {
                    app__win32WindowEvent(app_appEventType_iconified, window);
                } else {
                    app__win32WindowEvent(app_appEventType_restored, window);
                }
            }
        } break;
        case WM_SETFOCUS: {
            app__win32WindowEvent(app_appEventType_restored, window);
            app__win32WindowEvent(app_appEventType_focused, window);
        } break;
        case WM_KILLFOCUS: {
            /* if focus is lost for any reason, and we're in mouse locked mode, disable mouse lock */
            if (window->mouseLocked) {
                app__win32LockMouse(false);
            }
            app__win32WindowEvent(app_appEventType_unfocused, window);
        } break;
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                //_sapp_win32_update_cursor(_sapp.mouse.current_cursor, _sapp.mouse.shown, true);
                return TRUE;
            }
        } break;
        case WM_DPICHANGED: {
            /* Update window's DPI and size if its moved to another monitor with a different DPI
                Only sent if DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is used.
            */
            app__win32DpiChanged(window, (LPRECT)lParam);
            
        } break;
        case WM_LBUTTONDOWN:
            app__win32MouseUpdate(window, lParam);
            app__win32MouseEvent(window, app_appEventType_mouseDown, app_mouseButton_left);
            app__win32CaptureMouse(window, 1<<app_mouseButton_left);
            break;
        case WM_RBUTTONDOWN:
            app__win32MouseUpdate(window, lParam);
            app__win32MouseEvent(window, app_appEventType_mouseDown, app_mouseButton_right);
            app__win32CaptureMouse(window, 1<<app_mouseButton_right);
            break;
        case WM_MBUTTONDOWN:
            app__win32MouseUpdate(window, lParam);
            app__win32MouseEvent(window, app_appEventType_mouseDown, app_mouseButton_middle);
            app__win32CaptureMouse(window, 1<<app_mouseButton_middle);
            break;
        case WM_LBUTTONUP:
            app__win32MouseUpdate(window, lParam);
            app__win32MouseEvent(window, app_appEventType_mouseUp, app_mouseButton_left);
            app__win32ReleaseMouse(window, 1<<app_mouseButton_left);
            break;
        case WM_RBUTTONUP:
            app__win32MouseUpdate(window, lParam);
            app__win32MouseEvent(window, app_appEventType_mouseUp, app_mouseButton_right);
            app__win32ReleaseMouse(window, 1<<app_mouseButton_right);
            break;
        case WM_MBUTTONUP:
            app__win32MouseUpdate(window, lParam);
            app__win32MouseEvent(window, app_appEventType_mouseUp, app_mouseButton_middle);
            app__win32ReleaseMouse(window, 1<<app_mouseButton_middle);
            break;
        case WM_MOUSEMOVE:
            if (!window->mouse.locked) {
                app__win32MouseUpdate(window, lParam);
                if (!window->mouse.mouseTracked) {
                    window->mouse.mouseTracked = true;
                    TRACKMOUSEEVENT tme;
                    mem_setZero(&tme, sizeof(tme));
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = window->hWnd;
                    TrackMouseEvent(&tme);
                    window->mouse.dx = 0.0f;
                    window->mouse.dy = 0.0f;
                    app__win32MouseEvent(window, app_appEventType_mouseEnter, app_appEventType_invalid);
                }
                app__win32MouseEvent(window, app_appEventType_mouseMove, app_appEventType_invalid);
            }
            break;
        case WM_INPUT:
            /* raw mouse input during mouse-lock */
            if (window->mouse.locked) {
                HRAWINPUT ri = (HRAWINPUT) lParam;
                UINT size = sizeof(window->rawInputData);
                // see: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getrawinputdata
                if ((UINT)-1 == GetRawInputData(ri, RID_INPUT, &window->rawInputData, &size, sizeof(RAWINPUTHEADER))) {
                    //_SAPP_ERROR(WIN32_GET_RAW_INPUT_DATA_FAILED);
                    break;
                }
                const RAWINPUT* rawInputData = (const RAWINPUT*) &window->rawInputData;
                if (rawInputData->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                    /* mouse only reports absolute position
                        NOTE: This code is untested and will most likely behave wrong in Remote Desktop sessions.
                        (such remote desktop sessions are setting the MOUSE_MOVE_ABSOLUTE flag).
                        See: https://github.com/floooh/sokol/issues/806 and
                        https://github.com/microsoft/DirectXTK/commit/ef56b63f3739381e451f7a5a5bd2c9779d2a7555)
                    */
                    LONG new_x = rawInputData->data.mouse.lLastX;
                    LONG new_y = rawInputData->data.mouse.lLastY;
                    if (window->rawInputMouseposValid) {
                        window->mouse.dx = (float) (new_x - window->rawInputMouseposX);
                        window->mouse.dy = (float) (new_y - window->rawInputMouseposY);
                    }
                    window->rawInputMouseposX = new_x;
                    window->rawInputMouseposY = new_y;
                    window->rawInputMouseposValid = true;
                }
                else {
                    /* mouse reports movement delta (this seems to be the common case) */
                    window->mouse.dx = (float) rawInputData->data.mouse.lLastX;
                    window->mouse.dy = (float) rawInputData->data.mouse.lLastY;
                }
                app__win32MouseEvent(window, app_appEventType_mouseMove, app_appEventType_invalid);
            }
            break;

        case WM_MOUSELEAVE:
            if (!window->mouse.locked) {
                window->mouse.dx = 0.0f;
                window->mouse.dy = 0.0f;
                window->mouse.mouseTracked = false;
                app__win32MouseEvent(window, app_appEventType_mouseLeave, app_appEventType_invalid);
            }
            break;
        case WM_MOUSEWHEEL:
            app__win32ScrollEvent(window, 0.0f, (float)((SHORT)HIWORD(wParam)));
            break;
        case WM_MOUSEHWHEEL:
            app__win32ScrollEvent(window, (float)((SHORT)HIWORD(wParam)), 0.0f);
            break;
        case WM_CHAR:
            app__win32CharEvent(window, (uint32_t)wParam, !!(lParam&0x40000000));
            break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            app__win32KeyEvent(window, app_appEventType_keyDown, (int)(HIWORD(lParam)&0x1FF), !!(lParam&0x40000000));
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            app__win32KeyEvent(window, app_appEventType_keyUp, (int)(HIWORD(lParam)&0x1FF), false);
            break;
        case WM_ENTERSIZEMOVE:
            SetTimer(hWnd, 1, USER_TIMER_MINIMUM, NULL);
            break;
        case WM_EXITSIZEMOVE:
            KillTimer(hWnd, 1);
            break;
        case WM_TIMER:
            //_sapp_win32_timing_measure();
            //_sapp_frame();
            //#if defined(SOKOL_D3D11)
                // present with DXGI_PRESENT_DO_NOT_WAIT
               // _sapp_d3d11_present(true);
            //#endif
            //#if defined(SOKOL_GLCORE)
            //_sapp_wgl_swap_buffers();
            //#endif
            /* NOTE: resizing the swap-chain during resize leads to a substantial
                memory spike (hundreds of megabytes for a few seconds).

            if (_sapp_win32_update_dimensions()) {
                #if defined(SOKOL_D3D11)
                _sapp_d3d11_resize_default_render_target();
                #endif
                _sapp_win32_app_event(SAPP_EVENTTYPE_RESIZED);
            }
            */
            break;
        case WM_NCLBUTTONDOWN:
            /* workaround for half-second pause when starting to move window
                see: https://gamedev.net/forums/topic/672094-keeping-things-moving-during-win32-moveresize-events/5254386/
            */
            if (SendMessage(window->hWnd, WM_NCHITTEST, wParam, lParam) == HTCAPTION) {
                POINT point;
                GetCursorPos(&point);
                ScreenToClient(window->hWnd, &point);
                PostMessage(window->hWnd, WM_MOUSEMOVE, 0, ((uint32_t)point.x)|(((uint32_t)point.y) << 16));
            }
            break;
        case WM_DROPFILES:
            //_sapp_win32_files_dropped((HDROP)wParam);
            break;
        case WM_DISPLAYCHANGE:
            // refresh rate might have changed
            //_sapp_timing_reset(&_sapp.timing);
            break;

        default: break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
//     // Loop over all your windows here and update them
//     for (HWND window : allYourWindows) {
//         // Render to this window
//         RenderWindow(window);
//     }
// }

static const LPCWSTR* app__win32WindowClass = L"AppWindowClass";
void app_startApplication(void) {
    //WNDCLASSEXW* windowClass = &app__appCtx->windowClass;
    WNDCLASSW windowClass;
    mem_setZero(&windowClass, sizeof(windowClass));
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = (WNDPROC) os__win32DefaultWindowProc;
    windowClass.hInstance = GetModuleHandleW(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    windowClass.lpszClassName = L"AppWindowClass";
    RegisterClassW(&windowClass);
    // windowClass->cbSize        = sizeof(WNDCLASSEXA);
    // windowClass->cbWndExtra    = sizeof(void*);
    // windowClass->lpfnWndProc   = (WNDPROC) os__win32DefaultWindowProc;
    // windowClass->lpszClassName = L"AppWindowClass";
    // windowClass->hCursor       = LoadCursorA(NULL, MAKEINTRESOURCEA(32512));
    // windowClass->style         = CS_OWNDC | CS_DBLCLKS;

    RegisterClassW(&windowClass);

    if (app__appCtx->desc.init) {
        app__appCtx->desc.init();
    }
    
    bool done = false;

    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (WM_QUIT == msg.message) {
                done = true;
                continue;
            } else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        
        if (app__appCtx->desc.update) {
            app__appCtx->desc.update();
        }

    }
    
    while (1) {
        MSG msg;
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                if (app__appCtx->desc.cleanup) {
                    app__appCtx->desc.cleanup();
                }
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (app__appCtx->desc.update) {
            app__appCtx->desc.update();
        }
    }
}
void app_initApplication(app_ApplicationDesc* applicationDesc) {
    if (app__winCallBeforeDone != NULL) {
        return;
    }
    BaseMemory baseMem = os_getBaseMemory();
    mms appMemoryBudget = applicationDesc->appMemoryBudget == 0 ? MEGABYTE(20) : applicationDesc->appMemoryBudget;
    Arena* mainArena = mem_makeArena(&baseMem, appMemoryBudget);
    app__appCtx = mem_arenaPushStructZero(mainArena, app__AppCtx);
    app__appCtx->mainArena = mainArena;
    app__appCtx->desc = *applicationDesc;
    app__winCallBeforeDone = app_startApplication;
}

void app_stopApplication (void) {
    // destroy all windows...
}

void* app_getUserData(void) {
    return app__appCtx->desc.user;
}

void app_setUserData(void* userPtr) {
    app__appCtx->desc.user = userPtr;
}

u32 app_maxWindows(void) {
    return OS_MAX_WINDOWS;
}

// updates current window and framebuffer size from the window's client rect, returns true if size has changed
LOCAL bool app__win32UpdateDimensions(app__Window* window) {
    RECT rect;
    if (GetClientRect(window->hWnd, &rect)) {
        float window_width  = (float)(rect.right - rect.left) / window->dpi.windowScale;
        float window_height = (float)(rect.bottom - rect.top) / window->dpi.windowScale;
        window->width  = window_width; //(int)roundf(window_width);
        window->height = window_height; //(int)roundf(window_height);
        float fb_width  = window_width  * window->dpi.contentScale; //(int)roundf(window_width  * window->dpi.contentScale);
        float fb_height = window_height * window->dpi.contentScale; //(int)roundf(window_height * window->dpi.contentScale);
        /* prevent a framebuffer size of 0 when window is minimized */
        if (0 == fb_width) {
            fb_width = 1;
        }
        if (0 == fb_height) {
            fb_height = 1;
        }
        if ((fb_width != window->frameBufferWidth) || (fb_height != window->frameBufferHeight)) {
            window->frameBufferWidth = fb_width;
            window->frameBufferHeight = fb_height;
            return true;
        }
    } else {
        window->width = window->height = 1;
        window->frameBufferWidth = window->frameBufferHeight = 1;
    }
    return false;
}
#if 0
LOCAL void app__win32InitDpi(void) {
    DECLARE_HANDLE(DPI_AWARENESS_CONTEXT_T);
    typedef BOOL(WINAPI * SETPROCESSDPIAWARE_T)(void);
    typedef bool (WINAPI * SETPROCESSDPIAWARENESSCONTEXT_T)(DPI_AWARENESS_CONTEXT_T); // since Windows 10, version 1703
    typedef HRESULT(WINAPI * SETPROCESSDPIAWARENESS_T)(PROCESS_DPI_AWARENESS);
    typedef HRESULT(WINAPI * GETDPIFORMONITOR_T)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

    SETPROCESSDPIAWARE_T fn_setprocessdpiaware = 0;
    SETPROCESSDPIAWARENESS_T fn_setprocessdpiawareness = 0;
    GETDPIFORMONITOR_T fn_getdpiformonitor = 0;
    SETPROCESSDPIAWARENESSCONTEXT_T fn_setprocessdpiawarenesscontext =0;

    HINSTANCE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        fn_setprocessdpiaware = (SETPROCESSDPIAWARE_T)(void*) GetProcAddress(user32, "SetProcessDPIAware");
        fn_setprocessdpiawarenesscontext = (SETPROCESSDPIAWARENESSCONTEXT_T)(void*) GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    }
    HINSTANCE shcore = LoadLibraryA("shcore.dll");
    if (shcore) {
        fn_setprocessdpiawareness = (SETPROCESSDPIAWARENESS_T)(void*) GetProcAddress(shcore, "SetProcessDpiAwareness");
        fn_getdpiformonitor = (GETDPIFORMONITOR_T)(void*) GetProcAddress(shcore, "GetDpiForMonitor");
    }
    /*
        NOTE on SetProcessDpiAware() vs SetProcessDpiAwareness() vs SetProcessDpiAwarenessContext():

        These are different attempts to get DPI handling on Windows right, from oldest
        to newest. SetProcessDpiAwarenessContext() is required for the new
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 method.
    */
    if (fn_setprocessdpiawareness) {
        if (_sapp.desc.high_dpi) {
            /* app requests HighDPI rendering, first try the Win10 Creator Update per-monitor-dpi awareness,
               if that fails, fall back to system-dpi-awareness
            */
            _sapp.win32.dpi.aware = true;
            DPI_AWARENESS_CONTEXT_T per_monitor_aware_v2 = (DPI_AWARENESS_CONTEXT_T)-4;
            if (!(fn_setprocessdpiawarenesscontext && fn_setprocessdpiawarenesscontext(per_monitor_aware_v2))) {
                // fallback to system-dpi-aware
                fn_setprocessdpiawareness(PROCESS_SYSTEM_DPI_AWARE);
            }
        } else {
            /* if the app didn't request HighDPI rendering, let Windows do the upscaling */
            _sapp.win32.dpi.aware = false;
            fn_setprocessdpiawareness(PROCESS_DPI_UNAWARE);
        }
    } else if (fn_setprocessdpiaware) {
        // fallback for Windows 7
        _sapp.win32.dpi.aware = true;
        fn_setprocessdpiaware();
    }
    /* get dpi scale factor for main monitor */
    if (fn_getdpiformonitor && _sapp.win32.dpi.aware) {
        POINT pt = { 1, 1 };
        HMONITOR hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        UINT dpix, dpiy;
        HRESULT hr = fn_getdpiformonitor(hm, MDT_EFFECTIVE_DPI, &dpix, &dpiy);
        UNUSED(hr);
        ASSERT(SUCCEEDED(hr));
        /* clamp window scale to an integer factor */
        _sapp.win32.dpi.window_scale = (float)dpix / 96.0f;
    } else {
        _sapp.win32.dpi.window_scale = 1.0f;
    }

    if (_sapp.desc.high_dpi) {
        _sapp.win32.dpi.content_scale = _sapp.win32.dpi.window_scale;
        _sapp.win32.dpi.mouse_scale = 1.0f;
    } else {
        _sapp.win32.dpi.content_scale = 1.0f;
        _sapp.win32.dpi.mouse_scale = 1.0f / _sapp.win32.dpi.window_scale;
    }

    _sapp.dpi_scale = _sapp.win32.dpi.content_scale;

    if (user32) {
        FreeLibrary(user32);
    }

    if (shcore) {
        FreeLibrary(shcore);
    }
}
#endif

app_window app_makeWindow(app_WindowDesc* desc) {
    u32 idx = 0;
    app_window window;

    while (1) {
        if (countOf(app__appCtx->windows) < idx) {
            window.id = 0;
            return window;
        }
        if (app__appCtx->windows[idx].active != true) {
            break;
        }
    }

    if (idx == countOf(app__appCtx->windows)) {
        window.id = 0;
        ASSERT(!"No free window!");
        return window;
    }

    mem_defineMakeStackArena(windowNameArena, 500 * sizeof(u32));
    S8 windowName = desc->title.size == 0 ? s8("") : desc->title;
    S8 nullterminatedWindowName = str_join(windowNameArena, desc->title, s8("\0"));

    const DWORD winExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD winStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;

    int windowWidth = desc->width ? desc->width : 420;
    int windowHeight = desc->height ? desc->height : 320;

    app__Window* win = app__appCtx->windows + idx;
    win->dpi.aware = 1.0f;
    win->dpi.contentScale = 1.0f;
    win->dpi.windowScale = 1.0f;
    win->dpi.mouseScale = 1.0f;

    RECT rect = { 0, 0, 0, 0 };
    rect.right  = (int) ((float)windowWidth  * 1.0f/*win->dpi.window_scale*/);
    rect.bottom = (int) ((float)windowHeight * 1.0f/*win->dpi.window_scale*/);

    AdjustWindowRectEx(&rect, winStyle, FALSE, winExStyle);
    const int win_width = rect.right - rect.left;
    const int win_height = rect.bottom - rect.top;

    win->dpi.scale = 1.0f;

    win->width = windowWidth;
    win->height = windowHeight;
    win->frameBufferWidth = windowWidth;
    win->frameBufferHeight = windowHeight;
    win->active = true;
    win->hWnd = CreateWindowExW(
        winExStyle,                 // dwExStyle
        L"AppWindowClass", //app__win32WindowClass,      // lpClassName
        L"Window Title :(",//nullterminatedWindowName.content,    // lpWindowName
        winStyle,                   // dwStyle
        CW_USEDEFAULT,              // X
        SW_HIDE, //SW_HIDE,                    // Y (NOTE: CW_USEDEFAULT is not used for position here, but internally calls ShowWindow!
        windowWidth,                // nWidth
        windowHeight,               // nHeight (NOTE: if width is CW_USEDEFAULT, height is actually ignored)
        NULL,                       // hWndParent
        NULL,                       // hMenu
        GetModuleHandle(NULL),      // hInstance
        NULL                        // lParam
    );

    ASSERT(win->hWnd);
    
    win->dc = GetDC(win->hWnd);
    win->hmonitor = MonitorFromWindow(win->hWnd, MONITOR_DEFAULTTONULL);

    app__win32UpdateDimensions(win);
    
    ShowWindow(win->hWnd, SW_SHOW);
    DragAcceptFiles(win->hWnd, 1);
    
    
    //CreateWindowExA(0, app__win32WindowClass, nullterminatedWindowName.content, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, desc->width ? desc->width : 420, desc->height ? desc->height : 320, NULL, NULL, NULL, win);

    window.id = idx + 1;
    return window;
}

app__Window* app__getWindow(app_window window) {
    if (window.id == 0 || countOf(app__appCtx->windows) <= (window.id - 1)) {
        return NULL;
    }
    window.id -= 1;
    return &app__appCtx->windows[window.id];
} 

Vec2 app_getWindowSizeF32(app_window window) {
    Vec2 windowSize;
    windowSize.x = app__appCtx->windows[window.id - 1].width;
    windowSize.y = app__appCtx->windows[window.id - 1].height;

    return windowSize;
}

void app_showWindow(app_window window) {
    ASSERT(window.id);
    window.id -= 1;
    app__Window* win = &app__appCtx->windows[window.id];
    ShowWindow(win->hWnd, SW_SHOW);
}

void app_hideWindow(app_window window) {
    ASSERT(window.id);
    window.id -= 1;
    ShowWindow(app__appCtx->windows[window.id].hWnd, SW_HIDE);
}

void app_destroyWindow(app_window window) {
    ASSERT(window.id);
    window.id -= 1;
    app__appCtx->windows[window.id].active = false;
    DestroyWindow(app__appCtx->windows[window.id].hWnd);
}

void* app_getGraphicsHandle(app_window window) {
    app__Window* windowPtr = app__getWindow(window);
    if (windowPtr == NULL) {
        return NULL;
    }
    return (void*) windowPtr->hWnd;
}

void* app_getWin32WindowHandle(app_window window) {
    app__Window* windowPtr = app__getWindow(window);
    if (windowPtr == NULL) {
        return NULL;
    }
    return (void*) windowPtr->hWnd;
}

#elif OS_ANDROID
#error "ANDROID no implemented"
#else
#error "OS no implemented"
#endif


// Shared code

LOCAL void app__initEvent(app_appEventType type, app_window window) {
    mem_setZero(&app__appCtx->event, sizeof(app__appCtx->event));
    app__Window* windowPtr = app__getWindow(window);

    app__appCtx->event.type = type;
    if (windowPtr == NULL) {
        app__appCtx->event.window.id = 0;
        app__appCtx->event.windowSize.x = 0;
        app__appCtx->event.windowSize.y = 0;
        app__appCtx->event.frameBufferSize.x = 0;
        app__appCtx->event.frameBufferSize.y = 0;
        app__appCtx->event.mouse.pos.x = 0;
        app__appCtx->event.mouse.pos.y = 0;
        app__appCtx->event.mouse.delta.x = 0;
        app__appCtx->event.mouse.delta.y = 0;
    } else {
        app__appCtx->event.window.id = 0;
        app__appCtx->event.windowSize.x = windowPtr->width;
        app__appCtx->event.windowSize.y = windowPtr->height;
        app__appCtx->event.frameBufferSize.x = windowPtr->frameBufferWidth;
        app__appCtx->event.frameBufferSize.y = windowPtr->frameBufferHeight;
        app__appCtx->event.mouse.pos.x = windowPtr->mouse.x;
        app__appCtx->event.mouse.pos.y = windowPtr->mouse.y;
        app__appCtx->event.mouse.delta.x = windowPtr->mouse.dx;
        app__appCtx->event.mouse.delta.y = windowPtr->mouse.dy;
    }
    //app__appCtx->event.frameCount = windowPtr app__appCtx->frameCount;

    app__appCtx->event.mouse.button = app_mouseButton__invalid;
    //app__appCtx->event.frameCount = app__appCtx->frameCount;
}


#endif