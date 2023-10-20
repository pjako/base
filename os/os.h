
#ifndef OS_H
#define OS_H
#include <stdint.h>
#ifndef API
#define API extern
#endif
#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////
// os init func, must be called immediately but only if os_entry is not used
API void os_init(void);

/////////////////////////
// Memory Fuctions
#if OS_EMSCRIPTEN
#define OS_VIRTUAL_MEMORY 0
#else
#define OS_VIRTUAL_MEMORY 1
#endif // OS_EMSCRIPTEN

API umm os_getProcessMemoryUsed(void);
API u32 os_memoryPageSize(void);

API void* os_memoryReserve(u64 size);
API void  os_memoryCommit(void* ptr, u64 size);
API void  os_memorydecommit(void* ptr, u64 size);
API void  os_memoryRelease(void* ptr, u64 size);

API BaseMemory os_getBaseMemory(void);

/////////////////////////
// time related functionality
API u64 os_nowInMicroSeconds(void);


/////////////////////////
// executable path
#if 0
API Str8 os_execPath(Arena* arena);
#endif
API Str8 os_workingPath(Arena* arena);


/////////////////////////
// File handling

API Str8 os_fileRead(Arena* arena, Str8 fileName);
API bx os_fileWrite(Str8 fileName, Str8 data);
API bx os_fileDelete(Str8 fileName);
API bx os_fileExists(Str8 fileName);


/////////////////////////
// File properties

typedef enum os_DataAccessFlags {
    os_dataAccessFlag_read    = (1 << 0),
    os_dataAccessFlag_write   = (1 << 1),
    os_dataAccessFlag_execute = (1 << 2),
} os_DataAccessFlags;

typedef enum os_FilePropertyFlags {
    os_filePropertyFlag_isFolder = (1 << 0),
} os_FilePropertyFlags;

typedef struct os_FileProperties {
    os_FilePropertyFlags flags;
    os_DataAccessFlags access;
    u64 size;
    DenseTime creationTime;
    DenseTime lastChangeTime;
} os_FileProperties;

API os_FileProperties os_fileProperties(Str8 fileName);


/////////////////////////
// Filesystem watching

typedef struct os_fsPathWatchId {
    u32 id;
} os_fsPathWatchId;

typedef enum os_fsAction {
    os_fsAction_create = 1,
    os_fsAction_delete,
    os_fsAction_modifiy,
    os_fsAction_move
} os_fsAction;

typedef enum os_fsPathWatchFlag {os_fsPathTrackFlag_none = 0x0} os_fsPathWatchFlag;
typedef flags32 os_fsPathWatchFlags;

typedef void(os_fsPathWatchCallback)(os_fsPathWatchId id, Str8 path, os_fsAction fsAction, void* custom);

typedef struct os_fsPathWatchCtx os_fsPathWatchCtx;
API os_fsPathWatchId os_fsWatchPathStart(os_fsPathWatchCtx* ctx, Str8 folder, os_fsPathWatchFlags trackFlags, os_fsPathWatchCallback* callback, void* custom);
API void os_fsWatchPathStop(os_fsPathWatchCtx* ctx, os_fsPathWatchId handle);

API os_fsPathWatchCtx* os_fsWatchPathCreatCtx(Arena* arena);
API void os_fsWatchPathDestroyCtx(os_fsPathWatchCtx* ctx);
API void os_fsWatchPathTick(os_fsPathWatchCtx* ctx);


/////////////////////////
// Execute

API void* os_execute(Arena* tmpArena, Str8 execPath, Str8* args, u32 argCount);

/////////////////////////
// Dynamic library

typedef struct os_Dl os_Dl;

API os_Dl* os_dlOpen(Str8 filePath);
API void   os_dlClose(os_Dl* handle);
API void*  os_dlSym(os_Dl* handle, const char* symbol);

/////////////////////////
// Threading Related

API void os_sleep(u32 ms);
API void os_yield(void);

/////////////////////////
// Logging
void os_log(Str8 msg);

/////////////////////////
// Mutex
typedef struct os_Mutex {
    ALIGN_DECL(16, u8) internal[64];
} os_Mutex;

API void os_mutexInit(os_Mutex* mutex);
API void os_mutexDestroy(os_Mutex* mutex);
API void os_mutexLock(os_Mutex* mutex);
API bx   os_mutexTryLock(os_Mutex* mutex);
API void os_mutexUnlock(os_Mutex* mutex);

API u32 os__mutexLockRet(os_Mutex* mutex);
#define os_mutexScoped(mutex) for (u32 iii = (0, os__mutexLockRet(mutex), 0); iii == 0; (iii++, os_mutexUnlock(mutex)))

/////////////////////////
// Semaphores
typedef struct os_Semaphore {
    ALIGN_DECL(16, uint8_t) internal[128];
} os_Semaphore;

API void os_semaphoreInit(os_Semaphore* sem);
API void os_semaphorePost(os_Semaphore* sem, u32 count);
API bool os_semaphoreWait(os_Semaphore* sem, i32 count);
API void os_semaphoreDestroy(os_Semaphore* sem);

/////////////////////////
// Threads
struct os_Thread;
typedef i32 (os_threadFunc)(struct os_Thread* thread, void* userData);
typedef struct os_Thread {
    ALIGN_DECL(16, u8) internal[64];
    os_threadFunc*     entryFunc;
    void*              userData;
    os_Semaphore       sem;
    u32                stackSize;
    i32                exitCode;
    bool               running;
} os_Thread;

API bool os_threadCreate(os_Thread* thread, os_threadFunc threadFunc, void* userData, u32 stackSize, Str8 name);
API void os_threadShutdown(os_Thread* thread);
API bool os_threadIsRunning(os_Thread* thread);
API i32  os_threadGetExitCode(os_Thread* thread);
API void os_threadSetName(os_Thread* thread, Str8 str);

API void os_threadInit(void);
API void os_threadUninit(void);

/////////////////////////
// Fiber

typedef struct os_Fiber {
    ALIGN_DECL(16, u8) internal[64];
} os_Fiber;

typedef void (os_fiberEntryFunc) (void* ptr);

API void os_fiberInit(os_Fiber* fiber, os_fiberEntryFunc* entry, void *stackPointer, umm stackSize, void* userPtr);
// Can only be called from the a thread, not within a running fiber
API void os_fiberResume(os_Fiber* fiber);
// Yield the current fiber, will assert when it is called from the current thread
API void os_fiberYield(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // OS_H

