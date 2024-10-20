#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include <stdlib.h>


// std malloc

#if 0
typedef struct mem_StdMallocBlock {
    struct mem_StdMallocBlock* next;
    struct mem_StdMallocBlock* prev;
    u64 size;
    u8 mem[0];
} mem_StdMallocBlock;

typedef struct mem__StdMallocator {
    Allocator allocator;
	usize size     THREAD_GUARDED(mutex);
	usize deleted  THREAD_GUARDED(mutex);
	usize capacity THREAD_GUARDED(mutex);
	void **items  THREAD_GUARDED(mutex);
	os_Mutex mutex;
} mem__StdMallocator;

// Malloc "like" alloocator implementation


static const Size DEFAULT_CAPACITY = 4096;
static const Float32 RESIZE_THRESHOLD = 0.75;
static const Uint32 PRIME1 = 73;
static const Uint32 PRIME2 = 5009;

#define TOMBSTONE ((void*) 1)

mem__StdMallocator* mem_makeStdMallocator(void) THREAD_INTERNAL {
    usize capacity = DEFAULT_CAPACITY;
	mem__StdMallocator *allocator =  (mem__StdMallocator*) calloc(1, sizeOf(mem__StdMallocator));

	if (!allocator) {
		return NULL;
	}
    mem_setZero(allocator, sizeOf(mem__StdMallocator));

	os_mutexInit(&allocator->mutex);
	allocator->size = 0;
	allocator->capacity = DEFAULT_CAPACITY;
	allocator->items = (void**) calloc(allocator->capacity, sizeOf(allocator->items[0]));
	if (!allocator->items) {
		free(allocator);
		return NULL;
	}

	return allocator;
}


mem__StdMallocator* mem_destroyStdMallocator(Allocator* allocator) THREAD_INTERNAL {
    mem__StdMallocator* stdAlloc = (mem__StdMallocator*) allocator->allocator;
    os_mutexScoped(&stdAlloc->mutex) {
        for (usize i = 0; i < capacity; i++) {
            void *data = allocator->items[i];
            if (data == TOMBSTONE) {
                continue;
            }
            free(data);
            allocator->items[i] = TOMBSTONE;
            allocator->deleted++;
        }
    }
    os_mutexDestroy(&stdAlloc->mutex);
    free(stdAlloc);
}

API u32 os__mutexLockRet(os_Mutex* mutex);
#define os_mutexScoped(mutex) for (u32 iii = (0, os__mutexLockRet(mutex), 0); iii == 0; (iii++, os_mutexUnlock(mutex)))

static void std_allocator_fini(Allocator *ctx) {
	StandardAllocator *allocator = CAST(StandardAllocator *, ctx->user);

	mutex_lock(&allocator->mutex);
	const Size capacity = allocator->capacity;
	free(allocator->items);
	mutex_unlock(&allocator->mutex);

	mutex_fini(&allocator->mutex);
	free(allocator);
}

static bx std_allocator_add_unlocked(StandardAllocator *allocator, void *item);
static bx std_allocator_maybe_rehash_unlocked(StandardAllocator *allocator)
	THREAD_REQUIRES(allocator->mutex)
{
	if (allocator->size + allocator->deleted < CAST(Float32, allocator->capacity) * RESIZE_THRESHOLD) {
		return true;
	}

	Size capacity = allocator->capacity * 2;
	void **items = CAST(void**, calloc(capacity, sizeof *items));
	if (!items) {
		return false;
	}

	void **old_items = allocator->items;
	Size old_capacity = allocator->capacity;

	allocator->capacity = capacity;
	allocator->items = items;
	allocator->deleted = 0;
	allocator->size = 0;

	for (Size i = 0; i < old_capacity; i++) {
		// NOTE(dweiler): This cannot fail since the capacity is strictly greater.
		std_allocator_add_unlocked(allocator, old_items[i]);
	}

	free(old_items);

	return true;
}

static Bool std_allocator_add_unlocked(StandardAllocator *allocator, void *item)
	THREAD_REQUIRES(allocator->mutex)
{
	Uint64 hash = RCAST(Uint64, item); // TODO(dweiler): MixInt
	const Size mask = allocator->capacity - 1;

	Size index = (PRIME1 * hash) & mask;

	for (;;) {
		void *element = allocator->items[index];
		if (element && element != TOMBSTONE) {
			if (element == item) {
				return false;
			} else {
				index = (index + PRIME2) & mask;
			}
		} else {
			break;
		}
	}

	allocator->size++;
	allocator->items[index] = item;

	return std_allocator_maybe_rehash_unlocked(allocator);
}

static Bool std_allocator_add(StandardAllocator *allocator, void *item) {
	if (!item || item == TOMBSTONE) {
		return false;
	}

	mutex_lock(&allocator->mutex);
	const Bool result = std_allocator_add_unlocked(allocator, item);
	mutex_unlock(&allocator->mutex);
	return result;
}

static Bool std_allocator_remove(StandardAllocator *allocator, void *item) {
	u64 hash = RCAST(Uint64, item); // TODO(dweiler): MixInt

	mutex_lock(&allocator->mutex);

	const usize mask = allocator->capacity - 1;
	usize index = mask & (PRIME1 * hash);

	for (;;) {
		void *element = allocator->items[index];
		if (element) {
			if (element == item) {
				allocator->items[index] = TOMBSTONE;
				allocator->size--;
				allocator->deleted++;
				mutex_unlock(&allocator->mutex);
				return true;
			} else {
				index = mask & (index + PRIME2);
			}
		} else {
			break;
		}
	}

	mutex_unlock(&allocator->mutex);
	return false;
}

static Ptr std_allocator_allocate(Allocator *allocator, Size bytes) {
	void *data = malloc(bytes);
	if (!data) {
		return 0;
	}
	
	if (!std_allocator_add(CAST(StandardAllocator*, allocator->user), data)) {
		free(data);
		return 0;
	}
	
	return data;
}

LOCAL void std_allocator_reallocate(Allocator *allocator, void *data, Size old_size, Size new_size) {
	if (!data) {
		return 0;
	}

	if (new_size <= old_size) {
		return data;
	}

	StandardAllocator *std_allocator = CAST(StandardAllocator*, allocator->user);
	std_allocator_remove(std_allocator, data);

	void *resize = realloc(data, new_size);
	if (!resize) {
		return 0;
	}

	if (!std_allocator_add(std_allocator, resize)) {
		free(resize);
		return 0;
	}

	return resize;
}

static void std_allocator_deallocate(Allocator *allocator, void *data) {
	if (!data) {
		return;
	}
	std_allocator_remove(CAST(StandardAllocator*, allocator->user), data);
	free(data);
}


// Malloc "like" alloocator implementation 




LOCAL void* mem__stdAllocFn(u64 size, void* userPtr) {
    mem__StdMalloc* stdMalloc = (mem__StdMalloc*) userPtr;
    return mem_arenaPush(arena, size);
}

LOCAL void* mem__stdReallocFn(u64 size, void* oldPtr, u64 oldSize, void* userPtr) {
    mem__StdMalloc* stdMalloc = (mem__StdMalloc*) userPtr;
    void* newMem = mem_arenaPush(arena, size);

    mem_copy(newMem, oldPtr, oldSize);

    return newMem;
}

LOCAL void mem__stdFreeFn(void* ptr, void* userPtr) {
    mem__StdMalloc* stdMalloc = (mem__StdMalloc*) userPtr;
}

Allocator* mem_makeStdMalloc(BaseMemory* baseMemory, usize size, usize alignment) {
    mem__StdMalloc* stdMalloc = (mem__StdMalloc*) baseMemory->commit(baseMemory->ctx, NULL, sizeof(mem__StdMalloc));
    mem_structSetZero(stdMalloc);

    return stdMalloc;
}

void mem_destroyStdMalloc(Allocator* allocator) {
    ASSERT(allocator);
}
#endif

void* mem__reserve(void* ctx, u64 size) {
    unused(ctx);
    return malloc(size);
}

void mem__commit(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx, size);
}

void mem__decommit(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx, ptr, size);
}

void mem__release(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx, ptr, size);
}

BaseMemory mem_getMallocBaseMem(void) {
    BaseMemory baseMem;
    baseMem.ctx = NULL;
    baseMem.pageSize = OS_DEFAULT_PAGE_SIZE;
    baseMem.reserve = mem__reserve;
    baseMem.commit = mem__commit;
    baseMem.decommit = mem__decommit;
    baseMem.release = mem__release;

    return baseMem;
}


u64 mem_getArenaMemOffsetPos(Arena* arena) {
    u64 offset = ((u64) &arena->memory[0]) - ((u64) arena);
    return arena->pos - offset;
}

void* mem_arenaPush(Arena* arena, u64 size) {
    ASSERT(arena);
    void* result = NULL;
    if (!arena->unsafeRecord && arena->alignment > 1) {
        size = alignUp(size, arena->alignment);
    }
    if (arena->pos + size <= arena->cap) {
        result = arena->memory + (arena->pos - (u64_cast(&arena->memory[0]) - u64_cast(arena)));
        arena->pos += size;

        u64 p = arena->pos;
        u64 commitP = arena->commitPos;
        if (p > commitP) {
            u64 pAlign      = alignUp(p, arena->base.pageSize);
            u64 nextCommitP = clampTop(pAlign, arena->cap);
            u64 commitSize  = nextCommitP - commitP;
            arena->base.commit(arena->base.ctx, ((u8*) arena) + arena->commitPos, commitSize);
            arena->commitPos = nextCommitP;
        }
    }
    return result;
}

void mem_arenaPopTo(Arena* arena, u64 pos) {
    ASSERT(arena);
    pos = maxVal(sizeof(Arena), pos);
    if (pos < arena->pos) {
        arena->pos      = pos;

        u64 p           = arena->pos;
        u64 pAlign      = alignUp(p, arena->base.pageSize);
        u64 nextCommitP = clampTop(pAlign, arena->cap);
        u64 commitP     = arena->commitPos;
        if (nextCommitP < commitP) {
            u64 decommitSize = commitP - nextCommitP;
            arena->base.decommit(arena->base.ctx, ((u8*) arena) + nextCommitP, decommitSize);
            arena->commitPos = nextCommitP;
        }
    }
}
void mem_arenaPopAmount(Arena* arena, u64 amount) {
    mem_arenaPopTo(arena, arena->pos - amount);
}

mem_Scratch mem_scratchStart(Arena* arena) {
    mem_Scratch scratch;
    scratch.arena = arena;
    scratch.start = arena->pos;
    return scratch;
}

void mem_scratchEnd(mem_Scratch* scratch) {
    mem_arenaPopTo(scratch->arena, scratch->start);
}

typedef struct mem__MallocEntry {
    struct mem__MallocEntry* prev;
    struct mem__MallocEntry* next;
    bx      active : 1;
    u64     size;
    u8      mem[0];
} mem__MallocEntry;

typedef struct ManagedAlloc {
    Arena* arena;
    mem__MallocEntry* first;
    mem__MallocEntry* last;
    mem__MallocEntry* firstHole;
    u64 largestHole;
    u8 mem[0];
} ManagedAlloc;

ManagedAlloc* mem_makeManagedAlloc(Arena* arena) {
    ManagedAlloc* mallocCtx = mem_arenaPush(arena, sizeof(ManagedAlloc));
    mem_setZero(mallocCtx, sizeof(ManagedAlloc));
    mallocCtx->arena = arena;
    return mallocCtx;
}

void* mem_managedAlloc(ManagedAlloc* malloc, u64 size) {
    if (malloc->firstHole) {
        for (mem__MallocEntry* allocation = malloc->firstHole; allocation != NULL; allocation = allocation->next) {
            if (allocation->active) {
                // freeSize = 0;
                // prevFree = NULL;
                continue;
            }
            if (allocation->size >= size) {
                // todo: divide up the allocation when the unused mem is too big
                allocation->active = true;
                if (malloc->firstHole->active) {
                    mem__MallocEntry* firstHole = malloc->firstHole;
                    malloc->firstHole = NULL;
                    for (;firstHole;firstHole = firstHole->next) {
                        if (!firstHole->active) {
                            malloc->firstHole = firstHole;
                            break;
                        }
                    }
                }
                return &allocation->mem[0];
            }
        }
    }

    mem__MallocEntry* allocation = (mem__MallocEntry*) mem_arenaPush(malloc->arena, sizeof(mem__MallocEntry) + size);
    allocation->prev = NULL;
    if (!malloc->first) {
        malloc->first = allocation;
    }
    allocation->prev = malloc->last;
    if (!malloc->last) {
        malloc->last = allocation;
    }
    malloc->last = allocation;
    return &allocation->mem[0];
}


LOCAL void* arena__allocFn(u64 size, void* userPtr) {
    Arena* arena = (Arena*) userPtr;
    return mem_arenaPush(arena, size);
}

LOCAL void* arena__reallocFn(u64 size, void* oldPtr, u64 oldSize, void* userPtr) {
    Arena* arena = (Arena*) userPtr;
    void* newMem = mem_arenaPush(arena, size);

    mem_copy(newMem, oldPtr, oldSize);

    return newMem;
}

LOCAL void arena__freeFn(void* ptr, void* userPtr) {
    unusedVars(ptr, userPtr);
}

Arena* mem_makeArena(BaseMemory* baseMem, u64 cap) {
    return mem_makeArenaAligned(baseMem, cap, 16);
}

Arena* mem_makeArenaAligned(BaseMemory* baseMem, u64 cap, u64 aligment) {
    u32 arr = sizeOf(Arena);
    ASSERT(baseMem);
    ASSERT(baseMem->reserve);
    ASSERT(baseMem->commit);
    ASSERT(baseMem->decommit);
    ASSERT(baseMem->release);

    u64 p            = sizeof(Arena);
    u64 pAlign       = alignUp(p, baseMem->pageSize);
    u64 commitSize   = clampTop(pAlign, cap);

    void* mem = baseMem->reserve(NULL, cap);
    baseMem->commit(NULL, mem, commitSize);
    Arena* arena = (Arena*) mem;
    mem_structSetZero(arena);
    arena->allocator.alloc = arena__allocFn;
    arena->allocator.realloc = arena__reallocFn;
    arena->allocator.free = arena__freeFn;
    arena->allocator.allocator = arena;
    arena->base = *baseMem;
    arena->cap = cap;
    arena->commitPos = commitSize;
    arena->alignment = aligment;

    arena->pos = ((u64) &arena->memory[0]) - ((u64) arena);
    return arena;
}

void mem_destroyArena(Arena* arena) {
    ASSERT(arena);
    ASSERT(arena->base.release);
    ASSERT(arena->cap > 0);

    mms cap = arena->cap;
    arena->commitPos = 0;
    arena->cap = 0;
    arena->base.release(arena->base.ctx, (void*) arena, cap);
}


u64 mem_arenaStartUnsafeRecord(Arena* arena) {
    ASSERT(arena);
    arena->unsafeRecord += 1;
    return mem_getArenaMemOffsetPos(arena);
}
void mem_arenaStopUnsafeRecord(Arena* arena) {
    ASSERT(arena);
    ASSERT(arena->unsafeRecord > 0);
    arena->unsafeRecord -= 1;

    if (arena->unsafeRecord == 0) {
        arena->pos = alignUp(arena->pos, arena->alignment);
    }
}

LOCAL void* mem__reservePre(void* ctx, u64 size) {
    unusedVars(ctx, size);
    return ctx;
}

LOCAL void mem__commitPre(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx, size, ptr);
}

LOCAL void mem__decommitPre(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx, size, ptr);
}

LOCAL void mem__releasePre(void* ctx, void* ptr, u64 size) {
    unusedVars(ctx, size, ptr);
}

Arena* mem_makeArenaPreAllocated(void* mem, u64 size) {
    Arena* arena = (Arena*) mem;
    mem_setZero(arena, sizeof(Arena));
    arena->base.ctx = mem;
    arena->base.pageSize = size;
    arena->base.reserve = mem__reserve;
    arena->base.commit = mem__commitPre;
    arena->base.decommit = mem__decommitPre;
    arena->base.release = mem__releasePre;

    arena->cap = size;
    arena->unsafeRecord = 0;
    arena->alignment = 16;
    arena->commitPos = size;
    arena->pos = ((u64) &arena->memory[0]) - ((u64) arena);

    return arena;
}
