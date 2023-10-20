#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include <stdlib.h>

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

void* mem_managedRealloc(ManagedAlloc* malloc, u64 newSize, void* ptr) {

}

void mem_managedFree(ManagedAlloc* malloc, void* ptr) {
    mem__MallocEntry* entry = (mem__MallocEntry*) &((u8*) ptr)[-sizeof(mem__MallocEntry)];
    
}

Arena* mem_makeArena(BaseMemory* baseMem, u64 cap) {
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
    arena->base = *baseMem;
    arena->cap = cap;
    arena->commitPos = commitSize;

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
    arena->base.ctx = mem;
    arena->base.pageSize = size;
    arena->base.reserve = mem__reserve;
    arena->base.commit = mem__commitPre;
    arena->base.decommit = mem__decommitPre;
    arena->base.release = mem__releasePre;

    arena->cap = size;
    arena->commitPos = size;
    arena->pos = ((u64) &arena->memory[0]) - ((u64) arena);

    return arena;
}
