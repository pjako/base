#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_math.h"
#include "base/base_str.h"
#include "base/base_time.h"
#include "os/os.h"

#if OS_APPLE || OS_ANDROID || OS_UNIX || OS_LINUX
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <mach/mach.h>
#include <stdio.h>
#include <dispatch/dispatch.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <dlfcn.h>

#if OS_APPLE
#include <sys/types.h>
#include <pwd.h>
#endif


#ifdef OS_OSX
#define DLL_EXTENSION ".dylib"
#else 
#define DLL_EXTENSION ".so"
#endif

os_Dl* os_dlOpen(Str8 filePath) {
    u8 fileName[1024];
    u32 size = minVal(countOf(fileName) - 1, filePath.size);
    mem_copy(fileName, filePath.content, size);
    Str8 extension = str_lit(DLL_EXTENSION);
    if (!str_hasSuffix(filePath, extension)) {
        ASSERT(sizeOf(fileName) > extension.size + size);
        mem_copy(&fileName[size], extension.content, extension.size);
        size += extension.size;

    }
    ASSERT(sizeOf(fileName) > size + 1);
    fileName[size] = '\0';

    void* handle = dlopen((const char *) fileName, RTLD_NOW);

    return (os_Dl*) handle;
}

void os_dlClose(os_Dl* handle) {
    dlclose((void*)handle);
}

void*  os_dlSym(os_Dl* handle, const char* symbol) {
    return (void*) dlsym((void*) handle, symbol);
}

// See: https://github.com/jemalloc/jemalloc/blob/12cd13cd418512d9e7596921ccdb62e25a103f87/src/pages.c
// See: https://web.archive.org/web/20150730125201/http://blog.nervus.org/managing-virtual-address-spaces-with-mmap/
void* os_memoryReserve(u64 size) {
    void* ptr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    int mres = msync(ptr, size, MS_SYNC | MS_INVALIDATE);
    ASSERT(mres != -1);
    return ptr;
}

void os_memoryCommit(void* ptr, u64 size) {
    void* res = mmap(ptr, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED | MAP_ANON, -1, 0);
    ASSERT(res != MAP_FAILED);
    ASSERT(res == ptr);
    int mres = msync(res, size, MS_SYNC | MS_INVALIDATE);
    ASSERT(mres != -1);
}

API void os_memorydecommit(void* ptr, u64 size) {
    // instead of unmapping the address, we're just gonna trick 
    // the TLB to mark this as a new mapped area which, due to 
    // demand paging, will not be committed until used.
    void* res = mmap(ptr, size, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
    int mres = msync(ptr, size, MS_SYNC | MS_INVALIDATE);
    ASSERT(mres != -1);
}

void os_memoryRelease(void* ptr, u64 size) {
    int mres = msync(ptr, size, MS_SYNC);
    ASSERT(mres != -1);
    munmap(ptr, size);
}

LOCAL void* os__reserve(void* ctx, u64 size) {
    unusedVars(ctx);
    ASSERT(size > 0);
    return os_memoryReserve(size);
}

LOCAL void os__commit(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx);
    ASSERT(size > 0);
    os_memoryCommit(ptr, size);
}

LOCAL void os__decommit(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx);
    ASSERT(size > 0);
    os_memorydecommit(ptr, size);
}

LOCAL void os__release(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx);
    ASSERT(size > 0);
    os_memoryRelease(ptr, size);
}

BaseMemory os_getBaseMemory(void) {
    BaseMemory mem;
    mem.ctx = NULL;
    mem.pageSize = os_memoryPageSize();
    mem.reserve = os__reserve;
    mem.commit = os__commit;
    mem.decommit = os__decommit;
    mem.release = os__release;
    return mem;
}


// Time

LOCAL DateTime os__posixDateTimeFromSystemTime(struct tm* in, u16 ms) {
	DateTime result = {0};
	result.year   = in->tm_year;
	result.mon    = in->tm_mon;
	result.day    = in->tm_mday;
	result.hour   = in->tm_hour;
	result.min    = in->tm_min;
	result.sec    = in->tm_sec;
	result.msec   = ms;
	return result;
}

LOCAL struct timespec os__posixLocalSystemTimeFromDateTime(DateTime* in) {
	struct tm result_tm = {0};
	result_tm.tm_year = in->year;
	result_tm.tm_mon  = in->mon;
	result_tm.tm_mday = in->day;
	result_tm.tm_hour = in->hour;
	result_tm.tm_min  = in->min;
	result_tm.tm_sec  = in->sec;
	long ms = in->msec;
	time_t result_tt = timelocal(&result_tm);
	struct timespec result = { .tv_sec = result_tt, .tv_nsec = ms * 1000000 };
	return result;
}

LOCAL struct timespec os__posixUniversalSystemTimeFromDateTime(DateTime* in) {
	struct tm result_tm = {0};
	result_tm.tm_year = in->year;
	result_tm.tm_mon  = in->mon;
	result_tm.tm_mday = in->day;
	result_tm.tm_hour = in->hour;
	result_tm.tm_min  = in->min;
	result_tm.tm_sec  = in->sec;
	long ms = in->msec;
	time_t result_tt = timegm(&result_tm);
	struct timespec result = { .tv_sec = result_tt, .tv_nsec = ms * 1000000 };
	return result;
}

DateTime os_timeUniversalNow(void) {
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	struct tm *the_tm = localtime(&spec.tv_sec);
	return os__posixDateTimeFromSystemTime(the_tm, (u16)(spec.tv_nsec / 1000000));
}

DateTime os_timeLocalFromUniversal(DateTime* date_time) {
	struct timespec local = os__posixLocalSystemTimeFromDateTime(date_time);
	struct tm *the_tm = localtime(&local.tv_sec);
	return os__posixDateTimeFromSystemTime(the_tm, (u16)(local.tv_nsec / 1000000));
}

DateTime os_timeUniversalFromLocal(DateTime* date_time) {
	struct timespec univ = os__posixUniversalSystemTimeFromDateTime(date_time);
	struct tm *the_tm = localtime(&univ.tv_sec);
	return os__posixDateTimeFromSystemTime(the_tm, (u16)(univ.tv_nsec / 1000000));
}

u64 os_timeMicrosecondsNow(void) {
	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	u64 us = ((u64)ts.tv_sec * 1000000) + ((u64)ts.tv_nsec / 1000000);
    return us;
}


/// File/Dir

Str8 os_fileRead(Arena* arena, Str8 fileName) {
    os_FileProperties fileProps = os_fileProperties(fileName);
    if (fileProps.size == 0) {
        return str_lit("");
    }
    u8 path[255 + 4096 + 1];
    ASSERT(sizeof(path) > (fileName.size + 1));
    ASSERT(fileName.content);
    ASSERT(fileName.size > 0);
    mem_copy(path, fileName.content, fileName.size);
    path[fileName.size] = '\0';

    i32 fileHandle = open((const char*) path, O_RDONLY);
    if (fileHandle == -1) {
        return str_lit("");
    }
    void* mem = mem_arenaPush(arena, fileProps.size);
    bool result = read(fileHandle, mem, fileProps.size) == fileProps.size;
    close(fileHandle);
    return result ? str_fromCharPtr((u8*) mem, fileProps.size) : str_lit("");
}

bx os_fileWrite(Str8 fileName, Str8 data) {
    // max file length + max file path length + '\0'
    u8 path[255 + 4096 + 1];
    ASSERT(sizeof(path) > (fileName.size + 1));
    ASSERT(fileName.content);
    ASSERT(fileName.size > 0);
    mem_copy(path, fileName.content, fileName.size);
    path[fileName.size] = '\0';
    // overwrite existing file and don't leave any of its existing file content
    i32 fileHandle = open((const char*) path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR + S_IWUSR + S_IRGRP + S_IROTH);
    if (fileHandle == -1) {
        return false;
    }
    bool result = write(fileHandle, data.content, data.size) == data.size;
    close(fileHandle);
    return result;
}

bx os_fileDelete(Str8 fileName) {
    u8 path[255 + 4096 + 1];
    ASSERT(sizeof(path) > (fileName.size + 1));
    ASSERT(fileName.content);
    ASSERT(fileName.size > 0);
    mem_copy(path, fileName.content, fileName.size);
    path[fileName.size] = '\0';

    return remove((const char*) path) == 0;
}

bx os_fileExists(Str8 fileName) {
    u8 path[255 + 4096 + 1];
    ASSERT(sizeof(path) > (fileName.size + 1));
    ASSERT(fileName.content);
    ASSERT(fileName.size > 0);
    mem_copy(path, fileName.content, fileName.size);
    path[fileName.size] = '\0';
    return access((const char*) path, F_OK) == 0;
}




bx os_dirCreate(Str8 dirname) {
    mem_defineMakeStackArena(tmpMem, 1024 * sizeof(u32) + 1);
    if (!str_isNullTerminated(dirname)) {
        dirname = str_copyNullTerminated(tmpMem, dirname);
    }
	b32 result = true;
	// NOTE(voxel): Not sure what mode is actually a good default...
	size_t o = mkdir((const char*) dirname.content, S_IRUSR | S_IRGRP | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP);
	if (o == -1) result = false;
	return o == -1 ? false : true;
}

bx os_dirDelete(Str8 dirname) {
    mem_defineMakeStackArena(tmpMem, 1024 * sizeof(u32) + 1);
    if (!str_isNullTerminated(dirname)) {
        dirname = str_copyNullTerminated(tmpMem, dirname);
    }
	size_t o = rmdir((const char*) dirname.content);

	return o == -1 ? false : true;
}

Str8 os_filepath(Arena* arena, os_systemPath path) {
	Str8 result = {0};
	
	switch (path) {
		case os_systemPath_currentDir: {
            mms pushAmount = 1024 * sizeOf(u32);
            u8* mem = mem_arenaPush(arena, pushAmount);
			getcwd((char*)mem, pushAmount);
			result.size = strlen((const char*)mem) - 1;
            result.content = mem;
            mem_arenaPopAmount(arena, pushAmount - result.size);
		} break;
		case os_systemPath_binary: {
            mms pushAmount = 1024 * sizeOf(u32);
            u8* mem = mem_arenaPush(arena, pushAmount);
			readlink("/proc/self/exe", (char*)mem, pushAmount);
			u64 end = str_lastIndexOfChar(result, '/');
			result.size = end - 1;
            result.content = mem;
            mem_arenaPopAmount(arena, pushAmount - result.size);
		} break;
		case os_systemPath_userData: {
			char* buffer = getenv("HOME");

            if (!buffer) {
                struct passwd* pwd = getpwuid(getuid());
                if (pwd) {
                    buffer = pwd->pw_dir;
                }
            }
            if (buffer) {
                result.content = (u8*)buffer;
                result.size = strlen((const char*)buffer);
            }
		} break;
		case os_systemPath_tempData: {
			return str_lit("/tmp");
		} break;
	}
	
	return result;
}

os_FileProperties os_fileProperties(Str8 fileName) {
    // max file length + max file path length + '\0'
    u8 path[255 + 4096 + 1];
    ASSERT(sizeof(path) > (fileName.size + 1));
    ASSERT(fileName.content);
    ASSERT(fileName.size > 0);
    mem_copy(path, fileName.content, fileName.size);
    path[fileName.size] = '\0';
    struct stat stats;

    i32 result = stat((const char*)&path[0], &stats);
    ASSERT(result == 0);
    os_FileProperties fileProps = {0};
    fileProps.size = stats.st_size;
    // https://c-for-dummies.com/blog/?p=4101
    if (S_ISDIR(stats.st_mode)) {
        fileProps.flags |= os_filePropertyFlag_isFolder;
    }
    // S_ISDIR, true when the file is a directory
    // S_ISLNK, true when the file is a symbolic link
    // S_ISREG, true when the file is a plain ol’ “regular” file

    if (stats.st_mode & S_IRUSR) {
        fileProps.access |= os_dataAccessFlag_read;
    }

    if (stats.st_mode & S_IWUSR) {
        fileProps.access |= os_dataAccessFlag_write;
    }

    if (stats.st_mode & S_IXUSR) {
        fileProps.access |= os_dataAccessFlag_execute;
    }
    
    struct tm  time;
    // gmtime_r
    struct tm *tt = gmtime_r(&stats.st_birthtimespec.tv_sec, &time);

    DateTime dateTime;
    dateTime.year  = tt->tm_year;
    dateTime.mon   = tt->tm_mon;
    dateTime.day   = tt->tm_mday;
    dateTime.hour  = tt->tm_hour;
    dateTime.sec   = tt->tm_sec;
    dateTime.msec  = 0; //roundVal(stats.st_ctimespec , 1000000);
    fileProps.creationTime = tm_toDenseTime(dateTime);

    tt = gmtime_r(&stats.st_ctimespec.tv_sec, &time);
    //DateTime dateTime;
    dateTime.year  = tt->tm_year;
    dateTime.mon   = tt->tm_mon;
    dateTime.day   = tt->tm_mday;
    dateTime.hour  = tt->tm_hour;
    dateTime.sec   = tt->tm_sec;
    dateTime.msec  = 0; //roundVal(stats.st_ctimespec , 1000000);
    fileProps.lastChangeTime = tm_toDenseTime(dateTime);

    return fileProps;
}


void* os_execute(Arena* tmpArena, Str8 execPath, Str8* args, u32 argCount) {
    pid_t pid = fork();
    if (pid != 0) {
        return (void*)((uintptr_t)pid);
    }
    mem_scoped(scratch, tmpArena) {
        Str8 exec = str_join(scratch.arena, execPath, '\0');
        Str8 argsUnix;
        str_record(argsUnix, scratch.arena) {
            for (u32 idx = 0; idx < argCount; idx++) {
                str_join(scratch.arena, args[idx], '\0');
            }
        }
        char** argsArr = (char**) mem_arenaPush(scratch.arena, sizeof(char*) * (argCount + 1));
        uint32_t offset = 0;
        for (u32 idx = 0; idx < argCount; idx++) {
            argsArr[idx] = (char*) argsUnix.content + offset;
            offset = args[idx].size + 1;
        }
        argsArr[argCount] = NULL;

        int result = execvp((char*) exec.content, argsArr);
    }
    return NULL;
}

void os_sleep(u32 ms) {
    struct timespec req = { (time_t)ms/1000, (long)( (ms%1000)*1000000) };
    struct timespec rem = { 0, 0 };
    nanosleep(&req, &rem);
}

void os_yield(void) {
#ifdef __MACH__
    sched_yield();
#else
    pthread_yield();
#endif
}

void os_mutexLock(os_Mutex* mutex) {
    pthread_mutex_lock((pthread_mutex_t*) mutex->internal);
}

bx os_mutexTryLock(os_Mutex* mutex) {
    return pthread_mutex_trylock((pthread_mutex_t*) mutex->internal) == 0;
}

void os_mutexUnlock(os_Mutex* mutex) {
    pthread_mutex_unlock((pthread_mutex_t*) mutex->internal);
}

void os_mutexInit(os_Mutex* mutex) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init((pthread_mutex_t*) mutex->internal, &attr);
}

void os_mutexDestroy(os_Mutex* mutex) {
    pthread_mutex_destroy((pthread_mutex_t*) mutex->internal);
}

u32 os__mutexLockRet(os_Mutex* mutex) {
    os_mutexLock(mutex);
    return 0;
}

void os_log(Str8 msg) {
    if (msg.content == NULL || msg.size == 0) {
        return;
    }
    //os_mutexScoped(&os__ctx.logLock) {
#if OS_ANDROID
        __android_log_print(ANDROID_LOG_UNKNOWN, "os", "%.*s\n", msg.size, msg.content);
#else
        fwrite(msg.content, msg.size, 1, stdout);
        fwrite("\n", 1, 1, stdout);
#endif // OS_ANDROD 
    //}
}

typedef struct os__ThreadInternal {
    pthread_t handle;
    char* tmpThreadName;
} os__ThreadInternal;

static void* os_threadEntry(void* arg) {
    os_Thread* thread = (os_Thread*) arg;
    union {
        void* ptr;
        i32 i;
    } cast;
    if (((os__ThreadInternal*) &thread->internal)->tmpThreadName) {
#if OS_APPLE
        pthread_setname_np(((os__ThreadInternal*) &thread->internal)->tmpThreadName);
#else
        pthread_setname_np(((os__ThreadInternal*) &thread->internal)->handle, ((os__ThreadInternal*) &thread->internal)->tmpThreadName);
#endif
        ((os__ThreadInternal*) &thread->internal)->tmpThreadName = NULL;
    }
    os_semaphorePost(&thread->sem, 1);
    cast.i = thread->entryFunc(thread, thread->userData);
    return cast.ptr;
}

bx os_threadCreate(os_Thread* thread, os_threadFunc threadFunc, void* userData, u32 stackSize, Str8 name) {
    os__ThreadInternal* ti = (os__ThreadInternal*) thread->internal;

    int result;

    pthread_attr_t attr;
    result = pthread_attr_init(&attr);
    if (0 != result) {
        return false;
    }
    // See:
    // https://man7.org/linux/man-pages/man3/pthread_attr_setstacksize.3.html
    // PTHREAD_STACK_MIN (16384)
    stackSize = maxVal(stackSize, 16384);

    if (stackSize != 0) {
        result = pthread_attr_setstacksize(&attr, stackSize);

        if (result != 0) {
            return false;
        }
    }
    
    
    char buff[1024];
    if (name.size > 0) {
        u64 length = minVal(name.size, countOf(buff) - 1);
        mem_copy(buff, name.content, length);
        buff[length] = '\0';
        ti->tmpThreadName = &buff[0];
    } else {
        ti->tmpThreadName = NULL;
    }
    
    
    thread->entryFunc = threadFunc;
    thread->userData = userData;

    os_semaphoreInit(&thread->sem);
    result = pthread_create(&ti->handle, &attr, os_threadEntry, thread);
    if (result != 0) {
        return false;
    }
    
    thread->running = true;
    os_semaphoreWait(&thread->sem, 1);

    return true;
}

void os_threadShutdown(os_Thread* thread) {
    os__ThreadInternal* internal = (os__ThreadInternal*) thread->internal;
    union {
        void* ptr;
        i32 i;
    } cast;
    pthread_join(internal->handle, &cast.ptr);
    thread->exitCode = cast.i;
    internal->handle = 0;
    thread->running = false;
    os_semaphoreDestroy(&thread->sem);
}

Str8 os_workingPath(Arena* arena) {
    Str8 str;
    u32 maxSize = PATH_MAX * sizeof(char);
    char* cwd = mem_arenaPushArray(arena, char, PATH_MAX);
    if (getcwd(cwd, maxSize) != 0) {
        str.content = (u8*) cwd;
        str.size = strlen(cwd);
    } else {
        str.content = NULL;
        str.size = 0;
    }

    mem_arenaPopAmount(arena, maxSize - str.size);

    return str;
}
#if 0
Str8 os_execPath(Arena* arena) {
    i32 length, dirnameLength;
    char* path = NULL;

    length = wai_getExecutablePath(NULL, 0, &dirnameLength);
    if (length > 0) {
        path = mem_arenaPushArray(arena, char, length);
        wai_getExecutablePath(path, length, &dirnameLength);
    }
    Str8 str;
    str.content = (u8*) path;
    str.size = dirnameLength;

    return str;
}
#endif

u32 os_memoryPageSize(void) {
    return getpagesize();
}

umm os_getProcessMemoryUsed(void) {
#ifdef MACH_TASK_BASIC_INFO
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

    i32 const result = task_info(mach_task_self() , MACH_TASK_BASIC_INFO , (task_info_t)&info , &infoCount);
#else
    task_basic_info info;
    mach_msg_type_number_t infoCount = TASK_BASIC_INFO_COUNT;

    i32 const result = task_info(mach_task_self() , TASK_BASIC_INFO , (task_info_t)&info , &infoCount);

#endif // MACH_TASK_BASIC_INFO
    if (result != KERN_SUCCESS) {
        return 0;
    }

    return info.resident_size;
}

#if OS_APPLE 
typedef struct os__SemaphoreInternal {
    dispatch_semaphore_t handle;
} os__SemaphoreInternal;

void os_semaphoreInit(os_Semaphore* sem) {
    STATIC_ASSERT(sizeof(os_Semaphore) >= sizeof(os__SemaphoreInternal));
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;
    si->handle = dispatch_semaphore_create(0);
    ASSERT(si->handle != NULL && "dispatch_semaphore_create failed.");
}

void os_semaphorePost(os_Semaphore* sem, u32 count) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;

    for (u32 ii = 0; ii < count; ++ii) {
        dispatch_semaphore_signal(si->handle);
    }
}

bool os_semaphoreWait(os_Semaphore* sem, i32 count) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;

    dispatch_time_t dt = 0 > count ? DISPATCH_TIME_FOREVER : dispatch_time(DISPATCH_TIME_NOW, i64_cast(count)*1000000);
    return !dispatch_semaphore_wait(si->handle, dt);
}

void os_semaphoreDestroy(os_Semaphore* sem) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;
    dispatch_release(si->handle);
}

#else

typedef struct os__SemaphoreInternal {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    i32 count;
} os__SemaphoreInternal;

void os_semaphoreInit(os_Semaphore* sem) {
    STATIC_ASSERT(sizeof(os_Semaphore) >= sizeof(os__SemaphoreInternal));

    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;
    si->count = 0;

    int result;

    result = pthread_mutex_init(&si->mutex, NULL);
    ASSERT(result == 0);

    result = pthread_cond_init(&si->cond, NULL);
    ASSERT(result == 0);
}

void os_semaphorePost(os_Semaphore* sem, u32 count) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;

    int result = pthread_mutex_lock(&si->mutex);
    ASSERT(result == 0);

    for (u32 ii = 0; ii < count; ++ii) {
        result = pthread_cond_signal(&si->cond);
        ASSERT(0 == result);
    }

    si->count += count;

    result = pthread_mutex_unlock(&si->mutex);
    ASSERT(0 == result);

    UNUSED(result);
}

INLINE u64 os__timeSpecToNs(const struct timespec ts) {
    return ts.tv_sec * u64_val(1000000000) + ts.tv_nsec;
}

INLINE struct timespec os__toTimespecNs(struct timespec ts, uint64_t _nsecs) {
    ts.tv_sec  = _nsecs / u64_val(1000000000);
    ts.tv_nsec = _nsecs % u64_val(1000000000);
    return ts;
}

INLINE struct timespec os__timespecAdd(struct timespec ts, u64 msecs) {
    u64 ns = os__timeSpecToNs(ts);
    return os__toTimespecNs(ts, ns + msecs * u64_val(1000000));
}

bool os_semaphoreWait(os_Semaphore* sem, i32 count) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;

    i32 result = pthread_mutex_lock(&si->mutex);
    ASSERT(result == 0);

    if (count == -1) {
        while (0 == result && 0 >= si->count) {
            result = pthread_cond_wait(&si->cond, &si->mutex);
        }
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts = os__timespecAdd(ts, (u64) count);

        while (0 == result && 0 >= si->count) {
            result = pthread_cond_timedwait(&si->cond, &si->mutex, &ts);
        }
    }

    bx ok = result == 0;

    if (ok) {
        --si->count;
    }

    result = pthread_mutex_unlock(&si->mutex);
    ASSERT(result == 0);

    UNUSED(result);

    return ok;
}

void os_semaphoreDestroy(os_Semaphore* sem) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;

    int result;
    result = pthread_cond_destroy(&si->cond);
    ASSERT(result == 0);

    result = pthread_mutex_destroy(&si->mutex);
    ASSERT(result == 0);

    UNUSED(result);
}
#endif // OS_APPLE || OS_ANDROID || OS_UNIX || OS_LINUX

#elif OS_WIN
#define PATH_MAX (1024)

#include <stdio.h>
#include <Windows.h>
#include <psapi.h>
#include <Synchapi.h>
#include <processthreadsapi.h>
#ifndef alloca
#define alloca(S) _alloca(S)
#endif

u32 os_memoryPageSize(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

umm os_getProcessMemoryUsed(void) {
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
}

void* os_memoryReserve(u64 size) {
    void* result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}

void os_memoryCommit(void* ptr, u64 size) {
    VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}

void os_memorydecommit(void* ptr, u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

void os_memoryRelease(void* ptr, u64 size) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

DateTime os_win32DateTimeFromSystemTime(SYSTEMTIME *in){
    DateTime result;
    result.year = in->wYear;
    result.mon  = (u8) in->wMonth;
    result.day  = in->wDay;
    result.hour = in->wHour;
    result.min  = in->wMinute;
    result.sec  = in->wSecond;
    result.msec = in->wMilliseconds;
    return result;
}

DenseTime os_win32DenseTimeFromFileTime(FILETIME *fileTime){
    SYSTEMTIME systemTime = {0};
    FileTimeToSystemTime(fileTime, &systemTime);
    DateTime dateTime = os_win32DateTimeFromSystemTime(&systemTime);
    DenseTime result = tm_toDenseTime(dateTime);
    return result;
}

Str8 os_fileRead(Arena* arena, Str8 fileName) {
    Str8 result;

    HANDLE file = INVALID_HANDLE_VALUE;
    mem_scoped(scratch, arena) {
        S16 fileName16 = str_toS16(scratch.arena, fileName);
        file = CreateFileW((WCHAR*) fileName16.content, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    }

    
    if (file != INVALID_HANDLE_VALUE) {
        DWORD hiSize = 0;
        DWORD loSize = GetFileSize(file, &hiSize);
        u64 totalSize = (u64_cast(hiSize) << 32) | u64_cast(loSize);
        mem_scoped(scratch, arena) {
            u8* buffer = mem_arenaPush(scratch.arena, totalSize);
            u8* ptr = buffer;
            bool success = true;
            
            for (u64 totalToRead = 0;totalToRead < totalSize;) {
                DWORD toRead = (DWORD) totalSize;
                if (totalToRead > u32_max) {
                    toRead = u32_max;
                }
                DWORD  actualRead = 0;
                if (!ReadFile(file, ptr, toRead, &actualRead, 0)) {
                    success = false;
                    break;
                }
                ptr += actualRead;
                totalToRead += actualRead;
            }
            CloseHandle(file);
            if (success) {
                result.content = buffer;
                result.size = totalSize;
                return result;
            }
        }
    }
    result.content = NULL;
    result.size = 0;

    return result;
}

bool os_fileWrite(Str8 fileName, Str8 data) {
    // max file length + max file path length + '\0'
    mem_defineMakeStackArena(arena, 1024 * sizeOf(u32));
    
    bx result = false;
    S16 fileMame16 = str_toS16(arena, fileName);
    HANDLE file = CreateFileW((WCHAR*)fileMame16.content, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (file != INVALID_HANDLE_VALUE) {
        result = true;
        DWORD actualWrite = 0;
        if (!WriteFile(file, data.content, data.size, &actualWrite, 0)) {
            result = false;
        } else {
            result = actualWrite == data.size;
        }
        
        CloseHandle(file);
    }

    return result;
}


bx os_dirCreate(Str8 dirname) {
    mem_defineMakeStackArena(tmpMem, 1024 * sizeof(u32) + 1);
    S16 dirname16 = str_toS16(tmpMem, dirname);
	b32 result = CreateDirectoryW((WCHAR*) dirname16.content, 0);
	return result;
}

bx os_dirDelete(Str8 dirname) {
    mem_defineMakeStackArena(tmpMem, 1024 * sizeof(u32) + 1);
    S16 dirname16 = str_toS16(tmpMem, dirname);
	b32 result = RemoveDirectoryW((WCHAR*) dirname16.content);
	return result;
}

#if 0
static U_DateTime w32_date_time_from_system_time(SYSTEMTIME* in){
	U_DateTime result = {0};
	result.year   = in->wYear;
	result.month  = (u8)in->wMonth;
	result.day    = in->wDay;
	result.hour   = in->wHour;
	result.minute = in->wMinute;
	result.sec    = in->wSecond;
	result.ms     = in->wMilliseconds;
	return result;
}

static SYSTEMTIME w32_system_time_from_date_time(U_DateTime* in){
	SYSTEMTIME result    = {0};
	result.wYear         = in->year;
	result.wMonth        = in->month;
	result.wDay          = in->day;
	result.wHour         = in->hour;
	result.wMinute       = in->minute;
	result.wSecond       = in->sec;
	result.wMilliseconds = in->ms;
	return result;
}

static u64 w32_dense_time_from_file_time(FILETIME *file_time) {
	SYSTEMTIME system_time;
	FileTimeToSystemTime(file_time, &system_time);
	U_DateTime date_time = w32_date_time_from_system_time(&system_time);
	U_DenseTime result = U_DenseTimeFromDateTime(&date_time);
	return result;
}

static OS_FilePropertyFlags w32_prop_flags_from_attribs(DWORD attribs) {
	OS_FilePropertyFlags result = 0;
	if (attribs & FILE_ATTRIBUTE_DIRECTORY){
		result |= FileProperty_IsFolder;
	}
	return result;
}

static OS_DataAccessFlags w32_access_from_attributes(DWORD attribs) {
	OS_DataAccessFlags result = DataAccess_Read | DataAccess_Exec;
	if (!(attribs & FILE_ATTRIBUTE_READONLY)){
		result |= DataAccess_Write;
	}
	return result;
}
#endif

os_FileProperties os_fileProperties(Str8 fileName) {
    // max file length + max file path length + '\0'
    u8 path[255 + 4096 + 1];
    ASSERT(sizeof(path) > (fileName.size + 1));
    ASSERT(fileName.content);
    ASSERT(fileName.size > 0);
    mem_copy(path, fileName.content, fileName.size);
    path[fileName.size] = '\0';
    #if 0
	M_Scratch scratch = scratch_get();
	string_utf16 filename16 = S16_from_str8(&scratch.arena, filename);
	OS_FileProperties result = {0};
	WIN32_FILE_ATTRIBUTE_DATA attribs = {0};
	if (GetFileAttributesExW((WCHAR*)filename16.str, GetFileExInfoStandard,
							 &attribs)) {
		result.size = ((u64)attribs.nFileSizeHigh << 32) | (u64)attribs.nFileSizeLow;
		result.flags = w32_prop_flags_from_attribs(attribs.dwFileAttributes);
		result.create_time = w32_dense_time_from_file_time(&attribs.ftCreationTime);
		result.modify_time = w32_dense_time_from_file_time(&attribs.ftLastWriteTime);
		result.access = w32_access_from_attributes(attribs.dwFileAttributes);
	}
	return result;
    #endif

    #if 0
    M_ArenaTemp scratch = m_get_scratch(0, 0);
    String16 file_name16 = S16_from_str8(scratch.arena, file_name);
    
    // get attribs and convert to properties
    FileProperties result = {};
    WIN32_FILE_ATTRIBUTE_DATA attribs = {};
    if (GetFileAttributesExW((WCHAR*)file_name16.str, GetFileExInfoStandard,
                             &attribs)){
        result.size = ((U64)attribs.nFileSizeHigh << 32) | (U64)attribs.nFileSizeLow;
        result.flags = w32_prop_flags_from_attribs(attribs.dwFileAttributes);
        result.create_time = w32_dense_time_from_file_time(&attribs.ftCreationTime);
        result.modify_time = w32_dense_time_from_file_time(&attribs.ftLastWriteTime);
        result.access = w32_access_from_attributes(attribs.dwFileAttributes);
    }


    struct stat stats;

    i32 result = stat((const char*)&path[0], &stats);
    ASSERT(result == 0);
    os_FileProperties fileProps = {0};
    fileProps.size = stats.st_size;
    // https://c-for-dummies.com/blog/?p=4101
    if (S_ISDIR(stats.st_mode)) {
        fileProps.flags |= os_filePropertyFlag_isFolder;
    }
    // S_ISDIR, true when the file is a directory
    // S_ISLNK, true when the file is a symbolic link
    // S_ISREG, true when the file is a plain ol’ “regular” file

    if (stats.st_mode & S_IRUSR) {
        fileProps.access |= os_dataAccessFlag_read;
    }

    if (stats.st_mode & S_IWUSR) {
        fileProps.access |= os_dataAccessFlag_write;
    }

    if (stats.st_mode & S_IXUSR) {
        fileProps.access |= os_dataAccessFlag_execute;
    }
    
    struct tm  time;
    // gmtime_r
    struct tm *tt = gmtime_r(&stats.st_birthtimespec.tv_sec, &time);

    DateTime dateTime;
    dateTime.year  = tt->tm_year;
    dateTime.mon   = tt->tm_mon;
    dateTime.day   = tt->tm_mday;
    dateTime.hour  = tt->tm_hour;
    dateTime.sec   = tt->tm_sec;
    dateTime.msec  = 0; //roundVal(stats.st_ctimespec , 1000000);
    fileProps.creationTime = denseTime_fromDateTime(dateTime);

    tt = gmtime_r(&stats.st_ctimespec.tv_sec, &time);
    //DateTime dateTime;
    dateTime.year  = tt->tm_year;
    dateTime.mon   = tt->tm_mon;
    dateTime.day   = tt->tm_mday;
    dateTime.hour  = tt->tm_hour;
    dateTime.sec   = tt->tm_sec;
    dateTime.msec  = 0; //roundVal(stats.st_ctimespec , 1000000);
    fileProps.lastChangeTime = denseTime_fromDateTime(dateTime);

    return fileProps;
    #endif
}

Str8 os_filepath(Arena* arena, os_systemPath path) {
	Str8 result = {0};

    DWORD tmpName[2048 * 2];
    DWORD tmpCount = countOf(tmpName);
    S16 path16 = {0};
    path16.content = (u16*) tmpName;

    mem_defineMakeStackArena(tmpArena, 2048 * sizeOf(u32));

	switch (path) {
		case os_systemPath_currentDir: {
            mms pushAmount = 1024 * sizeOf(u32);
            u8* mem = mem_arenaPush(arena, pushAmount);
			DWORD size = GetCurrentDirectoryW(tmpCount, (WCHAR*) tmpName);
            ASSERT((size >= tmpCount) && "Increase tmpName size");
            path16.size = size;
            result = str_fromS16(tmpArena, path16);
        case os_systemPath_binary: {
            mms pushAmount = 1024 * sizeOf(u32);
            u8* mem = mem_arenaPush(arena, pushAmount);
            DWORD size = GetModuleFileNameW(0, (WCHAR*)try_buffer, cap);
            ASSERT(size == tmpCount && GetLastError() == ERROR_INSUFFICIENT_BUFFER && "Increase tmpName size");
            path16.size = size;
            Str8 fullPath = str_fromS16(tmpArena, path16);
			Str8 binaryPath = os_getDirectoryFromFilepath(fullPath);
            result = binaryPath;
		} break;
		case os_systemPath_userData: {
			HANDLE token = GetCurrentProcessToken();
            bx success = GetUserProfileDirectoryW(token, (WCHAR*)tmpName, &tmpCount);
            ASSERT(success && "Increase tmpName size");
            path16.size = tmpCount;
            result = str_fromS16(tmpArena, path16);
		} break;
		case os_systemPath_tempData: {
			DWORD size = GetTempPathW(cap, (WCHAR*)buffer);
            ASSERT(size >= tmpCount && "Increase tmpName size");
            path16.size = size;
            result = str_fromS16(tmpArena, path16);
		} break;
	}

    result = str_replaceAll(arena, result, str_lit("\\"), str_lit("/"));
	
	return result;
}

Str8 os_filepath(Arena* arena, os_systemPath path) {
	string result = {0};
	switch (path) {
		case SystemPath_CurrentDir: {
			M_Scratch scratch = scratch_get();
			DWORD cap = 2048;
			u16* buffer = arena_alloc_array(&scratch.arena, u16, cap);
			DWORD size = GetCurrentDirectoryW(cap, (WCHAR*) buffer);
			if (size >= cap) {
				scratch_reset(&scratch);
				buffer = arena_alloc_array(&scratch.arena, u16, size + 1);
				size = GetCurrentDirectoryW(size + 1, (WCHAR*) buffer);
			}
			result = str8_from_S16(&scratch.arena, (string_utf16) { buffer, size });
			result = str_replace_all(arena, result, str_lit("\\"), str_lit("/"));
			
			scratch_return(&scratch);
		} break;
		
		case SystemPath_Binary: {
			M_Scratch scratch = scratch_get();
			
			DWORD cap = 2048;
			u16 *buffer = 0;
			DWORD size = 0;
			for (u64 r = 0; r < 4; r += 1, cap *= 4){
				u16* try_buffer = arena_alloc_array(&scratch.arena, u16, cap);
				DWORD try_size = GetModuleFileNameW(0, (WCHAR*)try_buffer, cap);
				
				if (try_size == cap && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
					scratch_reset(&scratch);
				} else {
					buffer = try_buffer;
					size = try_size;
					break;
				}
			}
			
			string full_path = str8_from_S16(&scratch.arena, (string_utf16) { buffer, size });
			string binary_path = U_GetDirectoryFromFilepath(full_path);
			result = str_replace_all(arena, binary_path, str_lit("\\"), str_lit("/"));
			
			scratch_return(&scratch);
		} break;
		
		case SystemPath_UserData: {
			M_Scratch scratch = scratch_get();
			
			HANDLE token = GetCurrentProcessToken();
			DWORD cap = 2048;
			u16 *buffer = arena_alloc_array(&scratch.arena, u16, cap);
			if (!GetUserProfileDirectoryW(token, (WCHAR*)buffer, &cap)) {
				scratch_reset(&scratch);
				buffer = arena_alloc_array(&scratch.arena, u16, cap + 1);
				if (GetUserProfileDirectoryW(token, (WCHAR*)buffer, &cap)) {
					buffer = 0;
				}
			}
			
			if (buffer) {
				result = str8_from_S16(&scratch.arena, S16_cstring(buffer));
				result = str_replace_all(arena, result, str_lit("\\"), str_lit("/"));
			}
			
			scratch_return(&scratch);
		} break;
		
		case SystemPath_TempData: {
			M_Scratch scratch = scratch_get();
			DWORD cap = 2048;
			u16 *buffer = arena_alloc_array(&scratch.arena, u16, cap);
			DWORD size = GetTempPathW(cap, (WCHAR*)buffer);
			if (size >= cap){
				scratch_reset(&scratch);
				buffer = arena_alloc_array(&scratch.arena, u16, size + 1);
				size = GetTempPathW(size + 1, (WCHAR*)buffer);
			}
			result = str8_from_S16(&scratch.arena, (string_utf16) { buffer, size - 1 });
			result = str_replace_all(arena, result, str_lit("\\"), str_lit("/"));
			
			scratch_return(&scratch);
		} break;
	}
	
	return result;
}

void* os_execute(Arena* tmpArena, Str8 execPath, Str8* args, u32 argCount) {
    STARTUPINFOA si;
    mem_structSetZero(&si);
    si.cb = sizeof(STARTUPINFOA);

    PROCESS_INFORMATION pi;
    mem_structSetZero(&pi);
    bx ok = false;
    mem_scoped(scratch, tmpArena) {
        Str8 exec = str_join(scratch.arena, execPath, '\0');
        Str8 argsWin;
        str_record(argsWin, scratch.arena) {
            for (u32 idx = 0; idx < argCount; idx++) {
                str_join(scratch.arena, args[idx], ' ');
            }
            str_join(scratch.arena, '\0');
        }

        ok = !!CreateProcessA(exec.content
            , argsWin.content
            , NULL
            , NULL
            , false
            , 0
            , NULL
            , NULL
            , &si
            , &pi
        );
    }

    return ok ? pi.hProcess : NULL;
}

void os_mutexLock(os_Mutex* mutex) {
    EnterCriticalSection((LPCRITICAL_SECTION) &mutex->internal[0]);
}

u32 os__mutexLockRet(os_Mutex* mutex) {
    os_mutexLock(mutex);
    return 0;
}

void os_mutexUnlock(os_Mutex* mutex) {
    LeaveCriticalSection((LPCRITICAL_SECTION) &mutex->internal[0]);
}

void os_mutexInit(os_Mutex* mutex) {
    InitializeCriticalSection((LPCRITICAL_SECTION) &mutex->internal[0]);
}

void os_mutexDestroy(os_Mutex* mutex) {
    DeleteCriticalSection((LPCRITICAL_SECTION) &mutex->internal[0]);
}

void os_log(Str8 msg) {
    if (msg.content == NULL || msg.size == 0) {
        return;
    }
   // os_mutexScoped(&os__ctx.logLock) {
#if OS_ANDROID
        __android_log_print(ANDROID_LOG_UNKNOWN, "os", "%.*s", msg.size, msg.content);
#else
        fwrite(msg.content, msg.size, 1, stdout);
#if OS_WIN

#if 0
    // Fast/correct debug log for windows
    // LPCSTR lpOutputString
    size_t a_len = strlen(lpOutputString);

    int len = MultiByteToWideChar(CP_UTF8, 0, lpOutputString, (int) a_len, nullptr, 0);
    wchar_t* w_str = (wchar_t*) _malloca(len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, lpOutputString, (int)a_len, w_str, len);

    ULONG_PTR args[4] = { (ULONG_PTR)len + 1, (ULONG_PTR)w_str, a_len + 1, (ULONG_PTR)lpOutputString };
    RaiseException(0x4001000A, 0, 4, args); // DBG_PRINTEXCEPTION_WIDE_C
#endif
    if (msg.content[msg.size - 1] == '\0') {
        OutputDebugStringA((char*) msg.content);
    } else {
        mem_scoped(scratch, os_tempMemory()) {
            u8* buff = (u8*) mem_arenaPush(scratch.arena, msg.size + 1);
            mem_copy(buff, msg.content, msg.size);
            buff[msg.size] = '\0';
            OutputDebugStringA((char*) buff);
        }
    }
#endif // OS_WIN
#endif // OS_ANDROD 
    //}
}

typedef struct os__SemaphoreInternal {
    HANDLE handle;
} os__SemaphoreInternal;

void os_semaphoreInit(os_Semaphore* sem) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;
    si->handle = NULL;
    si->handle = CreateSemaphoreA(NULL, 0, LONG_MAX, NULL);
    ASSERT(si->handle != NULL);
}

void os_semaphorePost(os_Semaphore* sem, u32 count) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;
    ReleaseSemaphore(si->handle, count, NULL);
}

bool os_semaphoreWait(os_Semaphore* sem, i32 count) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;
    DWORD milliseconds = (0 > count) ? INFINITE : count;
    return WaitForSingleObject(si->handle, milliseconds) == WAIT_OBJECT_0;
}

void os_semaphoreDestroy(os_Semaphore* sem) {
    os__SemaphoreInternal* si = (os__SemaphoreInternal*) sem->internal;
    CloseHandle(si->handle);
}

typedef struct os__ThreadInternal {
    HANDLE handle;
    DWORD  threadId;
} os__ThreadInternal;

static DWORD WINAPI os__threadEntry(LPVOID arg) {
    os_Thread* thread = (os_Thread*) arg;
    os__ThreadInternal* internal = (os__ThreadInternal*) thread->internal;
    internal->threadId = GetCurrentThreadId();
    os_semaphorePost(&thread->sem, 1);
    i32 result = thread->entryFunc(thread, thread->userData);
    return result;
}

bool os_threadCreate(os_Thread* thread, os_threadFunc threadFunc, void* userData, u32 stackSize, Str8 name) {
    os__ThreadInternal* internal = (os__ThreadInternal*) thread->internal;

    os_semaphoreInit(&thread->sem);
    internal->handle = CreateThread(NULL, stackSize, (LPTHREAD_START_ROUTINE)threadFunc, thread, 0, NULL);
    if (internal->handle == NULL) {
        return false;
    }
    thread->running = true;
    os_semaphoreWait(&thread->sem, 1);

    if (name.size > 0) {
        os_threadSetName(thread, name);
    }

    return true;
}

void os_threadShutdown(os_Thread* thread) {
    os__ThreadInternal* internal = (os__ThreadInternal*) thread->internal;
    WaitForSingleObject(internal->handle, INFINITE);
    GetExitCodeThread(internal->handle, (DWORD*)&thread->exitCode);
    CloseHandle(internal->handle);
    internal->handle = INVALID_HANDLE_VALUE;
    thread->running = false;
}

bool os_threadIsRunning(os_Thread* thread) {
    return thread->running;
}

i32 os_threadGetExitCode(os_Thread* thread) {
    return thread->exitCode;
}

void os_threadSetName(os_Thread* thread, Str8 name) {
    os__ThreadInternal* internal = (os__ThreadInternal*) thread->internal;
    // Try to use the new thread naming API from Win10 Creators update onwards if we have it
    typedef HRESULT (WINAPI SetThreadDescriptionProc)(HANDLE, PCWSTR);
    SetThreadDescriptionProc* setThreadDescription = (SetThreadDescriptionProc*) os_dlSym((os_Dl*)GetModuleHandleA("Kernel32.dll"), "SetThreadDescription");
    u32 totalSize = name.size + 1;
    char* nameBuffer = alloca(totalSize);
    mem_copy(nameBuffer, name.content, name.size);
    nameBuffer[name.size] = '\0';

    if (SetThreadDescription != NULL) {
        wchar_t* buff = alloca(totalSize);
        u32 size = totalSize * sizeof(wchar_t);
        mbstowcs(buff, name.content, size/* - 2*/);
        setThreadDescription(internal->handle, buff);
        return;
    }

#if COMPILER_MSVC
#pragma pack(push, 8)
    typedef struct ThreadName {
        DWORD  type;
        LPCSTR name;
        DWORD  id;
        DWORD  flags;
    } ThreadName;
#pragma pack(pop)
    ThreadName tn;
    tn.type  = 0x1000;
    tn.name  = nameBuffer;
    tn.id    = internal->threadId;
    tn.flags = 0;

    __try
    {
        RaiseException(0x406d1388, 0, sizeof(tn)/4, (const ULONG_PTR*)(&tn));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
#endif // COMPILER_MSVC
}

os_Dl* os_dlOpen(Str8 filePath) {
    HINSTANCE hInst;
    char fileName[1024];
    u32 size = minVal(countOf(fileName) - 1, filePath.size);
    mem_copy(fileName, filePath.content, size);
    fileName[size] = '\0';
    hInst = LoadLibrary(fileName);
#if 0
    if (hInst==NULL) {
        var.lasterror = GetLastError ();
        var.err_rutin = "dlopen";
    }
#endif
    return (os_Dl*) hInst;
}

DateTime os_timeUniversalNow(void) {
	SYSTEMTIME system_time;
	GetSystemTime(&system_time);
	DateTime result = w32_date_time_from_system_time(&system_time);
	return result;
}

DateTime os_timeLocalFromUniversal(DateTime* date_time) {
	SYSTEMTIME univ_system_time = w32_system_time_from_date_time(date_time);
	FILETIME univ_file_time;
	SystemTimeToFileTime(&univ_system_time, &univ_file_time);
	FILETIME local_file_time;
	FileTimeToLocalFileTime(&univ_file_time, &local_file_time);
	SYSTEMTIME local_system_time;
	FileTimeToSystemTime(&local_file_time, &local_system_time);
	DateTime result = w32_date_time_from_system_time(&local_system_time);
	return result;
}

DateTime os_timeUniversalFromLocal(DateTime* date_time) {
	SYSTEMTIME local_system_time = w32_system_time_from_date_time(date_time);
	FILETIME local_file_time;
	SystemTimeToFileTime(&local_system_time, &local_file_time);
	FILETIME univ_file_time;
	LocalFileTimeToFileTime(&local_file_time, &univ_file_time);
	SYSTEMTIME univ_system_time;
	FileTimeToSystemTime(&univ_file_time, &univ_system_time);
	DateTime result = w32_date_time_from_system_time(&univ_system_time);
	return result;
}

u64 os_timeMicrosecondsNow(void) {
	u64 result = 0;
	LARGE_INTEGER perf_counter = {0};
	if (QueryPerformanceCounter(&perf_counter)) {
		u64 ticks = ((u64)perf_counter.HighPart << 32) | perf_counter.LowPart;
		result = ticks * 1000000 / w32_ticks_per_sec;
	}
	return result;
}

// DLL

void os_dlClose(os_Dl* handle) {
    //BOOL ok;
    int rc= 0;

    //ok = FreeLibrary((HINSTANCE)handle);
    FreeLibrary((HINSTANCE)handle);
#if 0
    if (! ok) {
        var.lasterror = GetLastError ();
        var.err_rutin = "dlclose";
        rc= -1;
    }
#endif
}

void*  os_dlSym(os_Dl* handle, const char* symbol) {
    FARPROC fp;

    fp= GetProcAddress((HINSTANCE)handle, symbol);
#if 0
    if (!fp) {
        var.lasterror = GetLastError ();
        var.err_rutin = "dlsym";
    }
#endif
    return (void *)(intptr_t)fp;
}

void os_sleep(u32 ms) {
    Sleep(ms);
}

void os_yield(void) {
    YieldProcessor();
}

#else
#error "Unknown OS"
#endif