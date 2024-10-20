#ifndef APP_H
#define APP_H
#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////
// User entry point
// NOTE(pjako): This function has to be implemented by you,
//              It will be called by the os internal entry point

#ifndef OS_NO_ENTRY
extern i32 app_main(i32 argc, char* argv[]);

//#ifdef OS_RELOAD
extern void app_continue(void);
//#endif

#endif // OS_NO_ENTRY
////////////////////////
// Application / Window / Events

typedef struct app_window {
    u32 id;
} app_window;

typedef enum app_EventType {
    app_eventType__invalid = 0,
    app_eventType_shouldClose,
    app_eventType_shouldCloseWindow,
} app_EventType;

typedef enum app_mouseButton {
    app_mouseButton__invalid = 0,
    app_mouseButton_left = 1,
    app_mouseButton_middle = 2,
    app_mouseButton_right = 3,
    app_mouseButton__count,
} app_mouseButton;

enum {
    app_inputModifier_shift               = 0x1,      // left or right shift key
    app_inputModifier_ctrl                = 0x2,      // left or right control key
    app_inputModifier_alt                 = 0x4,      // left or right alt key
    app_inputModifier_super               = 0x8,      // left or right 'super' key
    app_inputModifier_leftMouseButton     = 0x100,    // left mouse button
    app_inputModifier_rightMuseButton     = 0x200,    // right mouse button
    app_inputModifier_middleMouseButton   = 0x400,    // middle mouse button
};

typedef enum app_keyCode {
    app_keyCode_invalid          = 0,
    app_keyCode_space            = 32,
    app_keyCode_apostrophe       = 39,  /* ' */
    app_keyCode_comma            = 44,  /* , */
    app_keyCode_minus            = 45,  /* - */
    app_keyCode_period           = 46,  /* . */
    app_keyCode_slash            = 47,  /* / */
    app_keyCode_0                = 48,
    app_keyCode_1                = 49,
    app_keyCode_2                = 50,
    app_keyCode_3                = 51,
    app_keyCode_4                = 52,
    app_keyCode_5                = 53,
    app_keyCode_6                = 54,
    app_keyCode_7                = 55,
    app_keyCode_8                = 56,
    app_keyCode_9                = 57,
    app_keyCode_semicolon        = 59,  /* ; */
    app_keyCode_equal            = 61,  /* = */
    app_keyCode_A                = 65,
    app_keyCode_B                = 66,
    app_keyCode_C                = 67,
    app_keyCode_D                = 68,
    app_keyCode_E                = 69,
    app_keyCode_F                = 70,
    app_keyCode_G                = 71,
    app_keyCode_H                = 72,
    app_keyCode_I                = 73,
    app_keyCode_J                = 74,
    app_keyCode_K                = 75,
    app_keyCode_L                = 76,
    app_keyCode_M                = 77,
    app_keyCode_N                = 78,
    app_keyCode_O                = 79,
    app_keyCode_P                = 80,
    app_keyCode_Q                = 81,
    app_keyCode_R                = 82,
    app_keyCode_S                = 83,
    app_keyCode_T                = 84,
    app_keyCode_U                = 85,
    app_keyCode_V                = 86,
    app_keyCode_W                = 87,
    app_keyCode_X                = 88,
    app_keyCode_Y                = 89,
    app_keyCode_Z                = 90,
    app_keyCode_leftBracket      = 91,  /* [ */
    app_keyCode_backslash        = 92,  /* \ */
    app_keyCode_rightBracket     = 93,  /* ] */
    app_keyCode_gravenAccent     = 96,  /* ` */
    app_keyCode_world1           = 161, /* non-US #1 */
    app_keyCode_world2           = 162, /* non-US #2 */
    app_keyCode_escape           = 256,
    app_keyCode_enter            = 257,
    app_keyCode_tab              = 258,
    app_keyCode_backspace        = 259,
    app_keyCode_insert           = 260,
    app_keyCode_delete           = 261,
    app_keyCode_right            = 262,
    app_keyCode_left             = 263,
    app_keyCode_down             = 264,
    app_keyCode_up               = 265,
    app_keyCode_pageUp           = 266,
    app_keyCode_pageDown         = 267,
    app_keyCode_home             = 268,
    app_keyCode_end              = 269,
    app_keyCode_capsLock         = 280,
    app_keyCode_scrollLock       = 281,
    app_keyCode_numLock          = 282,
    app_keyCode_printScreen      = 283,
    app_keyCode_pause            = 284,
    app_keyCode_F1               = 290,
    app_keyCode_F2               = 291,
    app_keyCode_F3               = 292,
    app_keyCode_F4               = 293,
    app_keyCode_F5               = 294,
    app_keyCode_F6               = 295,
    app_keyCode_F7               = 296,
    app_keyCode_F8               = 297,
    app_keyCode_F9               = 298,
    app_keyCode_F10              = 299,
    app_keyCode_F11              = 300,
    app_keyCode_F12              = 301,
    app_keyCode_F13              = 302,
    app_keyCode_F14              = 303,
    app_keyCode_F15              = 304,
    app_keyCode_F16              = 305,
    app_keyCode_F17              = 306,
    app_keyCode_F18              = 307,
    app_keyCode_F19              = 308,
    app_keyCode_F20              = 309,
    app_keyCode_F21              = 310,
    app_keyCode_F22              = 311,
    app_keyCode_F23              = 312,
    app_keyCode_F24              = 313,
    app_keyCode_F25              = 314,
    app_keyCode_keypad0          = 320,
    app_keyCode_keypad1          = 321,
    app_keyCode_keypad2          = 322,
    app_keyCode_keypad3          = 323,
    app_keyCode_keypad4          = 324,
    app_keyCode_keypad5          = 325,
    app_keyCode_keypad6          = 326,
    app_keyCode_keypad7          = 327,
    app_keyCode_keypad8          = 328,
    app_keyCode_keypad9          = 329,
    app_keyCode_keypadDecimal    = 330,
    app_keyCode_keypadDivide     = 331,
    app_keyCode_keypadMultiply   = 332,
    app_keyCode_keypadSubtract   = 333,
    app_keyCode_keypadAdd        = 334,
    app_keyCode_keypadEnter      = 335,
    app_keyCode_keypadEqual      = 336,
    app_keyCode_leftShift        = 340,
    app_keyCode_leftControl      = 341,
    app_keyCode_leftAlt          = 342,
    app_keyCode_leftSuper        = 343,
    app_keyCode_rightShift       = 344,
    app_keyCode_rightControl     = 345,
    app_keyCode_rightAlt         = 346,
    app_keyCode_rightSuper       = 347,
    app_keyCode_menu             = 348,
    app_keyCodes__count          = 512
} app_keyCode;

typedef enum app_appEventType {
    app_appEventType_invalid,
    app_appEventType_keyDown,
    app_appEventType_keyUp,
    app_appEventType_char,
    app_appEventType_mouseDown,
    app_appEventType_mouseUp,
    app_appEventType_mouseScroll,
    app_appEventType_mouseMove,
    app_appEventType_mouseEnter,
    app_appEventType_mouseLeave,
    //app_appEventType_touchesBegan,
    //app_appEventType_touchesMoved,
    //app_appEventType_touchesEnded,
    //app_appEventType_touchesCancelled,
    app_appEventType_resized,
    app_appEventType_iconified,
    app_appEventType_restored,
    app_appEventType_focused,
    app_appEventType_unfocused,
    app_appEventType_suspended,
    app_appEventType_resumed,

    //app_appEventType_quitRequested,
    //app_appEventType_pathChanged,
    //app_appEventType_filesDropped,
    app_appEventType_clipboardPasted,

    app_appEventType__count,
    app_appEventType__force32 = 0x7FFFFFFF
} app_appEventType;

typedef struct app_AppEvent {
    app_appEventType type;
    app_window window;
    app_window focusedWindow;
    app_keyCode keyCode;
    u32 charCode;
    bx keyRepeat;
    flags32 modifiers;
    u64 frameCount;
    Vec2 windowSize;
    Vec2 frameBufferSize;
    struct {
        Vec2 delta;
        Vec2 pos;
        Vec2 scroll;
        app_mouseButton  button;
    } mouse;
#if 0
    struct {
        S8 path;
        app_fsAction action;
    } pathChange;
#endif
} app_AppEvent;

typedef struct app_ApplicationDesc {
    void* user;
    mms appMemoryBudget;
    void (*init) (void);
    void (*update) (void);
    void (*event) (const app_AppEvent* event);
    void (*prepareReloadDll) (void);
    void (*continueDll) (void);
    void (*cleanup) (void);
    void (*fatalError) (S8 errorMsg);

    // coro threads directly in application code?
    // maybe threadpools as well?
    //bx useCoroJobs;
    //i32 coroThreadCount;
    //i32 coroMaxJobCount;
    //void* coroContext;
    //i32 maxCustomThreads;
} app_ApplicationDesc;

API void app_initApplication(app_ApplicationDesc* applicationDesc);
#ifdef OS_NO_ENTRY
API void app_startApplication(void);
API void app_stopApplication (void);
#endif

typedef enum app_windowGfxApi {
    app_windowGfxApi_openGl,
    //app_windowGfxApi_metal,
} app_windowGfxApi;

typedef enum app_windowDpi {
    app_windowDpi_systemDefault = 0,
    app_windowDpi_low,
    app_windowDpi_high
} app_windowDpi;

typedef struct app_WindowDesc {
    S8 title;
    app_windowGfxApi gfxApi;
    app_windowDpi dpi;
    u32 width;
    u32 height;
    u32 sampleCount;
    u32 x;
    u32 y;
} app_WindowDesc;

API void*       app_getUserData(void);
API void        app_setUserData(void* userPtr);

// init and clean thread local app structures (should not need be called on the mainthread)
API void        app_appInitThread(void);
API void        app_appCleanupThread(void);
API void        app_appNextFrameThread(void);

API u32         app_maxWindows(void);
API app_window  app_makeWindow(app_WindowDesc* desc);
API void        app_destroyWindow(app_window window);
API void        app_showWindow(app_window window);
API void        app_hideWindow(app_window window);
API void*       app_getWin32WindowHandle(app_window window);
API void*       app_getGraphicsHandle(app_window window);

API Vec2i       app_getWindowSize(app_window window);
API Vec2        app_getWindowSizeF32(app_window window);
// this includes dpi scale
API Vec2        app_getWindowFrameBufferSizeF32(app_window window);

API void        app_lockMouse(bx lock);
API bx          app_isMouseLocked(void);

// thread local memory that is valid the current & the next frame
API Arena*      app_frameArena(void);

typedef struct app_threadId {
    u32 id;
} app_threadId;

API app_threadId    app_startThread(void(*entry)(i32 id, void* userData), void* userData);
API void            app_stopThread(app_threadId id);

// Filewatch (based on file watch in os.h)
#if 0
API app_fsPathWatchId   app_fsWatchPathStart(S8 folder, app_fsPathWatchFlags trackFlags);
API void                app_fsWatchPathStop(app_fsPathWatchId handle);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // APP_H