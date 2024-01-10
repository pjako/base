#ifndef _BASE_MEM_
#define _BASE_MEM_

#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

#define mem_setZero(PTR, SIZE) memset((void*) (PTR), 0x0, (SIZE))
#define mem_structSetZero(PTR) mem_setZero((void*)(PTR), sizeof(*PTR))
#define mem_arrSetZero(PTR, COUNT) mem_setZero((PTR), sizeof((PTR)[0]) * (COUNT))

#define mem_copy(TO, FROM, SIZE) memcpy(TO, FROM, SIZE)

typedef void* (mem_reserveFunc)(void* ctx, u64 size);
typedef void  (mem_changeMemoryFunc)(void* ctx, void* ptr, u64 size);

typedef struct BaseMemory {
    void* ctx;
    u32 pageSize;
    mem_reserveFunc* reserve;
    mem_changeMemoryFunc* commit;
    mem_changeMemoryFunc* decommit;
    mem_changeMemoryFunc* release;
} BaseMemory;

typedef struct Arena {
    BaseMemory base;
    u64 cap;
    u64 pos;
    u64 commitPos;
    u8  memory[0];
} Arena;

API BaseMemory mem_getMallocBaseMem(void);

API Arena* mem_makeArena(BaseMemory* baseMem, u64 size);
API Arena* mem_makeArenaPreAllocated(void* mem, u64 size);
API void  mem_destroyArena(Arena* arena);

API u64 mem_getArenaMemOffsetPos(Arena* arena);

API void* mem_arenaPush(Arena* arena, u64 size);

#define mem_arenaPushZero(ARENA, SIZE) mem_setZero(mem_arenaPush(ARENA, SIZE), SIZE)
#define mem_arenaPushStruct(ARENA, STRUCT) (STRUCT*) mem_arenaPush(ARENA, sizeof(STRUCT))
#define mem_arenaPushStructZero(ARENA, STRUCT) (STRUCT*) mem_setZero(mem_arenaPush(ARENA, sizeof(STRUCT)), sizeof(STRUCT))
#define mem_arenaPushArray(ARENA, STRUCT, COUNT) (STRUCT*) mem_arenaPush(ARENA, sizeof(STRUCT) * COUNT)
#define mem_arenaPushArrayZero(ARENA, STRUCT, COUNT) (STRUCT*) mem_setZero(mem_arenaPush(ARENA, sizeof(STRUCT) * COUNT), sizeof(STRUCT) * COUNT)
#define mem_areaPushData(ARENA, DATA) mem_copy(mem_arenaPush(ARENA, sizeof(DATA)), &DATA, sizeof(DATA))

API void  mem_arenaPopTo(Arena* arena, u64 amount);
API void  mem_arenaPopAmount(Arena* arena, u64 amount);

typedef struct mem_Scratch {
    Arena* arena;
    u64 start;
} mem_Scratch;

API mem_Scratch mem_scratchStart(Arena* arena);
API void mem_scratchEnd(mem_Scratch* scratch);
#define mem_scoped(NAME, ARENA) for (mem_Scratch NAME = mem_scratchStart(ARENA);(NAME).arena; (mem_scratchEnd(&NAME), (NAME).arena = NULL))

#endif // _BASE_MEM_