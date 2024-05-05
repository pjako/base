#ifndef BASE_ATOMIC
#define BASE_ATOMIC

#ifdef _MSC_VER
#include <intrin.h>
//typedef u32 volatile A32;
//typedef u64 volatile A64;
#define a8_compareAndSwap(dst, expected, desired)  ((u8)  _InterlockedCompareExchange8((volatile char*)dst, (char)desired, (char)expected))
#define a16_compareAndSwap(dst, expected, desired) ((u16) _InterlockedCompareExchange16((volatile short*)dst, (short)desired, (short)expected))
#define a32_compareAndSwap(dst, expected, desired) ((u32) _InterlockedCompareExchange32((volatile long*)dst, (long)desired, (long)expected))
#define a64_compareAndSwap(dst, expected, desired) ((u64) _InterlockedCompareExchange64((volatile long long*)dst, (long long)desired, (long long)expected))
#define a32_loadAcquire(VALPTR) InterlockedOr32(VALPTR, 0)
#define a64_loadAcquire(VALPTR) InterlockedOr64(VALPTR, 0)

#define a32_add(dst, val) _InterlockedExchangeAdd((a32*) dst, (u32) val)
#define a64_add(dst, val) _InterlockedExchangeAdd64((a64*) dst, (u32) val)

// these intrinsics are x86 only
#define u32_bitScanNonZero(BITFIELD) u32_cast(_lzcnt_u32(BITFIELD))
#define u64_bitScanNonZero(BITFIELD) u64_cast(_lzcnt_u64(BITFIELD))
#define u32_bitScanReverseNonZero(BITFIELD) u32_cast(_tzcnt_u32(BITFIELD))
#define u64_bitScanReverseNonZero(BITFIELD) u64_cast(_tzcnt_u64(BITFIELD))
#define u32_popCount(BITFIELD) u32_cast(__popcnt(BITFIELD))
#define u64_popCount(BITFIELD) u64_cast(_mm_popcnt_u64(BITFIELD))

#define a32_add(dst, val) _InterlockedExchangeAdd((a32*) dst, (u32) val)
#define a64_add(dst, val) _InterlockedExchangeAdd64((a64*) dst, (u32) val)

#else
#include <stdatomic.h>
//typedef _Atomic(u32) A32;
//typedef _Atomic(u64) A64;
// these are clang/GCC specific
#define a8_compareAndSwap(dst, expected, desired)       ((u8 ) __sync_val_compare_and_swap(dst, expected, desired))
#define a16_compareAndSwap(dst, expected, desired)      ((u16) __sync_val_compare_and_swap(dst, expected, desired))
#define a32_compareAndSwap(dst, expected, desired)      ((u32) __sync_val_compare_and_swap(dst, expected, desired))
#define a64_compareAndSwap(dst, expected, desired)      ((u64) __sync_val_compare_and_swap(dst, expected, desired))
#define a32_loadAcquire(VALPTR)  __atomic_load_n(VALPTR, __ATOMIC_SEQ_CST)
#define a64_loadAcquire(VALPTR)  __atomic_load_n(VALPTR, __ATOMIC_SEQ_CST)
#define a32_add(dst, val) __atomic_fetch_add((u32*) dst, (u32) val, __ATOMIC_SEQ_CST)
#define a64_add(dst, val) __atomic_fetch_add((u64*) dst, (u64) val, __ATOMIC_SEQ_CST)

#define u32_bitScanNonZero(BITFIELD) u32_cast(__builtin_clz(BITFIELD))
#define u64_bitScanNonZero(BITFIELD) u64_cast(__builtin_clzll(BITFIELD))
#define u32_bitScanReverseNonZero(BITFIELD) u32_cast(__builtin_ctz(BITFIELD))
#define u64_bitScanReverseNonZero(BITFIELD) u64_cast(__builtin_ctzll(BITFIELD))
#define u32_popCount(BITFIELD) u32_cast(__builtin_popcount(BITFIELD))
#define u64_popCount(BITFIELD) u64_cast(__builtin_popcountll(BITFIELD))
#endif

typedef struct a32_MPSCIndexQueue {
    a32* elements;
    u64 capacity;
    a64 in;
    a64 out;
} a32_MPSCIndexQueue;

INLINE void a32_mpscInit(a32_MPSCIndexQueue* queue, Arena* arena, u64 capacity) {
    ASSERT(queue);
    ASSERT(arena);
    ASSERT(capacity > 0);
    queue->elements = mem_arenaPushArrayZero(arena, a32, capacity);
    queue->capacity = capacity;
    queue->in = 0;
    queue->out = 0;
}

#define a32_mpscForEach(QUEUE, INDEXNAME) for(u64 to = a32_loadAcquire(&(QUEUE)->in), INDEXNAME = (QUEUE)->out; (INDEXNAME) < to; (INDEXNAME)++)

INLINE void a32_mpscEnqeue(a32_MPSCIndexQueue* queue, a32 element) {
    ASSERT(element != 0);
    a64 index = a64_add(&queue->in, 1);
    queue->elements[index] = element;
}


INLINE u32 a32_mpscGetAtIndex(a32_MPSCIndexQueue* queue, u64 index) {
    ASSERT(queue->in  > index);
    ASSERT(queue->out <= index);
    u32* val = (u32*) &queue->elements[index];
    uintptr_t ptr = (uintptr_t) val;
    ASSERT(ptr % 4 == 0);
    return a32_loadAcquire(val);
}

INLINE u32 a32_mpscRemoveAtIndex(a32_MPSCIndexQueue* queue, u64 index) {
    ASSERT(queue->in  > index);
    ASSERT(queue->out <= index);
    u32 result = queue->elements[index];
    if (result == 0) {
        return 0;
    }

    if (index == queue->out) {
        queue->elements[index] = 0;
    } else {
        // move index at ount
        queue->elements[index] = queue->elements[queue->in];
        queue->elements[queue->in] = 0;
    }
    queue->out += 1;
    return result;
}

typedef struct a64_MpscRing {
    u64 size;
    a64 front;
    a64 back;
} a64_MpscRing;

INLINE void a64_mpscRingInit(a64_MpscRing* ring, u64 size) {
    ring->size = size;
    ring->front = 0;
    ring->back = 0;
}

INLINE i64 a64_mpscRingPush(a64_MpscRing* ring, u64 amount) {
    if (amount > ring->size) {
        return -1;
    }
    while (1) {
        // multiple threads could push
        a64 backValue = a64_loadAcquire(&ring->back);
        a64 startVal = a64_loadAcquire(&ring->front);

        //if (((startVal + amount) % ring->size) >= (backValue % ring->size)) {
        //    // amount does not fit in the ring buffer
        //    return -1;
        //}
        
        u64 actualStartVal = a64_compareAndSwap(&ring->front, startVal, startVal + amount);

        if (actualStartVal == startVal) {
            return startVal;
        }
    }
    return -1;
}

INLINE void a64_mpscRingPop(a64_MpscRing* ring, u64 amount) {
    ASSERT(ring->size >= amount);
    // this is done by a single thread
    a64_add(&ring->back, amount);
}

#endif /* BASE_ATOMIC */