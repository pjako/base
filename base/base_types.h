#ifndef _BASE_TYPES_
#define _BASE_TYPES_

////////////////////////////////
// NOTE(pjako): basic types
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t umm;
typedef size_t mms;
#define umm_cast(V) ((umm) (V))

#define u8_cast(V)  ((u8)  (V))
#define u16_cast(V) ((u16) (V))
#define u32_cast(V) ((u32) (V))
#define u64_cast(V) ((u64) (V))

#define u8_val(val)  val
#define u16_val(val) val
#define u32_val(val) val##ul
#define u64_val(val) val##ull

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

#define i8_cast(V)  ((i8)  (V))
#define i16_cast(V) ((i16) (V))
#define i32_cast(V) ((i32) (V))
#define i64_cast(V) ((i64) (V))

#define i8_val(val)  val
#define i16_val(val) val
#define i32_val(val) val##l
#define i64_val(val) val##ll


// bx should only used internally since its size is compiler dependent (per c spec)
#ifdef __cplusplus
typedef bool    bx;
#else
typedef _Bool    bx;
#endif
typedef u8       b8;
typedef u16      b16;
typedef u32      b32;

#define bx_cast(B)  ((b8) ((B) != 0))
#define b8_cast(B)  ((b8) ((B) != 0))
#define b16_cast(B) ((b16)((B) != 0))
#define b32_cast(B) ((b32)((B) != 0))

typedef u32      flags32;
typedef u64      flags64;

#define flag32(n) ((1u) << u32_cast(n))
#define flag64(n) ((1ull) << u64_cast(n))

typedef u32 volatile a32;
typedef u64 volatile a64;

typedef union f16 { u16 val; struct {i32 sign : 1; u32 exponent : 5 ; u32 mantissa : 10;}; } f16;
typedef float    f32;
typedef double   f64;

#define f32_cast(F) ((f32) (F))
#define f64_cast(F) ((f64) (F))

#ifdef SIMD_USE_SSE
#include <xmmintrin.h>

typedef union f16x4 {
    ALIGN_DECL(16, f16) values[4];
} f16x4;
typedef union f32x4 {
    __m128 simd;
    ALIGN_DECL(16, f32) values[4];
} f32x4;
typedef union i32x4 {
    __m128i simd;
    ALIGN_DECL(16, i32) values[4];
} i32x4;
typedef struct f32x16 {
    f32x4 _rows[4];
} f32x16;
typedef union f16x4 {
    ALIGN_DECL(16, f16) values[4];
} f16x4;


#elif defined(SIMD_USE_NEON)

typedef union f16x4 {
    float16x4_t simd;
    ALIGN_DECL(16, f16) values[4];
} f16x4;
typedef union f32x4 {
    float32x4_t simd;
    ALIGN_DECL(16, f32) values[4];
} f32x4;
typedef union i32x4 {
    int32x4_t simd;
    ALIGN_DECL(16, i32) values[4];
} i32x4;
typedef union f32x16 {
    f32x4 rows[4];
} f32x16;
typedef union f16x4 {
    float16x4_t simd;
    ALIGN_DECL(16, f16) values[4];
} f16x4;

#else

typedef union f32x4 {
    //ALIGN_DECL(16, f32) values[4];
    f32 values[4];
} f32x4;
typedef union i32x4 {
    //ALIGN_DECL(16, i32) values[4];
    i32 values[4];
} i32x4;
typedef union f32x16 {
    f32x4 rows[4];
} f32x16;
typedef union f16x4 {
    //ALIGN_DECL(16, f16) values[4];
    f16 values[4];
} f16x4;
#endif

typedef union Simd128 {
    f32x4 f32x4;
    i32x4 i32x4;
} Simd128;

typedef union Quat {
   f32 store[4];
   f32x4 simd;
} Quat;

typedef union Vec2 {
    struct {
        f32 x, y;
    };
    struct {
        f32 width, height;
    };
   f32 store[2];
#if CPP_ENV
    inline f32 &operator[](const int &index) {
        return store[maxVal(0, minVal(index, countOf(store)))];
    }
#endif
} Vec2;

typedef union Vec2i {
    struct {
        i32 x, y;
    };
   i32 store[2];
#if CPP_ENV
    inline i32 &operator[](const int &index) {
        return store[maxVal(0, minVal(index, countOf(store)))];
    }
#endif
} Vec2i;

typedef union Vec3 {
    struct {
        f32 x, y, z;
    };
    f32 store[3];
#if CPP_ENV
    inline f32 &operator[](const int &index) {
        return store[maxVal(0, minVal(index, countOf(store)))];
    }
#endif
} Vec3;

typedef union Vec4 {
    struct {
        f32 x, y, z, w;
    };
    struct {
        Vec2 origin;
        Vec2 size;
    };
    struct {
        Vec2 xy;
        Vec2 zw;
    };
    f32 store[4];
    f32x4 simd;
#if CPP_ENV
    inline f32 &operator[](const int &index) {
        return store[maxVal(0, minVal(index, countOf(store)))];
    }
#endif
} Vec4;

typedef union Mat4 {
    struct {
        f32 m00, m01, m02, m03;
        f32 m10, m11, m12, m13;
        f32 m20, m21, m22, m23;
        f32 m30, m31, m32, m33;
    };
    struct {
        Vec4 row0, row1, row2, row3;
    };
    f32 store[16];
    f32x16 simd;
#if CPP_ENV
    inline f32 &operator[](const int &index) {
        return store[maxVal(0, minVal(index, countOf(store)))];
    }
#endif
} Mat4;

typedef union Mat43 {
    struct {
        f32 m00, m01, m02;
        f32 m10, m11, m12;
        f32 m20, m21, m22;
        f32 m30, m31, m32;
    };
    struct {
        Vec3 row0, row1, row2, row3;
    };
   f32 store[16];
#if CPP_ENV
    inline f32 &operator[](const int &index) {
        return store[maxVal(0, minVal(index, countOf(store)))];
    }
#endif
} Mat43;

typedef union Rgba8 {
    struct {
        u8 red;
        u8 green;
        u8 blue;
        u8 alpha;
    };
    u32 uRgba;
    f32 fRgba;
    u8 store[4];
#if CPP_ENV
    inline u8 &operator[](const int &index) {
        return store[index];
    }
#endif
} Rgba8;

typedef union Rgb8 {
    struct {
        u8 red;
        u8 green;
        u8 blue;
    };
    u8 store[3];
#if CPP_ENV
    inline u8 &operator[](const int &index) {
        return store[index];
    }
#endif
} Rgb8;

typedef union Rgba {
    struct {
        f32 red;
        f32 green;
        f32 blue;
        f32 alpha;
    };
    f32 store[4];
#if CPP_ENV
    inline f32 &operator[](const int &index) {
        return store[index];
    }
#endif
} Rgba;


#pragma mark - String

typedef struct Str8 {
    u8* content;
    u64 size;
#if CPP_ENV
    inline u8 operator[](const int &index) {
        if (content == NULL || size == 0) {
            ASSERT(!"str is empty");
            return 0;
        }
        return content[maxVal(0, minVal(index, (size - 1)))];
    }
#endif
} Str8;

#ifdef __cplusplus
#define str_lit(STR) {(u8*) STR, sizeof(STR) - 1}
#define str_dyn(NAME, SIZE) u8 __strContent##__COUNT__ Str8 NAME = {&__strContent##__COUNT__[0], SIZE}
#else
#define str_lit(STR) (Str8) {(u8*) STR, sizeof(STR) - 1}
#define str_dyn(NAME, SIZE) u8 __strContent##__COUNT__ Str8 NAME = (Str8) {&__strContent##__COUNT__[0], SIZE}
#endif

typedef struct S16 {
    u16* content;
    u64  size;
#if CPP_ENV
    inline u16 operator[](const int &index) {
        if (content == NULL || size == 0) {
            return 0;
        }
        return content[maxVal(0, minVal(index, (size - 1)))];
    }
#endif
} S16;
#undef S32
typedef struct S32 {
    u32* content;
    u64  size;
#if CPP_ENV
    inline u32 operator[](const int &index) {
        if (content == NULL || size == 0) {
            return 0;
        }
        return content[maxVal(0, minVal(index, (size - 1)))];
    }
#endif
} S32;

typedef struct str_handle {
    u32 id;
} str_handle;


#pragma mark - Time

typedef struct DenseTime {
    u64 value;
} DenseTime;

typedef struct DateTime {
    u16 msec; // 0-999
    u8  sec;  // 0-60
    u8  min;  // 0-59
    u8  hour; // 0-23
    u8  day;  // 0-30
    u8  mon;  // 0-11
    i16 year; // 1 = 1 CE, 2020 = 2020 CE, -100 = 101 VCE, ...
} DateTime;

typedef union tm_FrequencyInfo {
    u64 frequency;
    struct {
        u32 numer;
        u32 denom;
    };
} tm_FrequencyInfo;


#pragma mark - Memory

typedef struct BaseMemory BaseMemory;
typedef struct Arena Arena;
typedef struct ManagedAlloc ManagedAlloc;

typedef void*(mem_allocFn)(u64 size, void* userPtr);
typedef void*(mem_reallocFn)(u64 size, void* oldPtr, void* userPtr);
typedef void*(mem_freeFn)(void* ptr, void* userPtr);


#pragma mark - Containers

#define arrView(ARR) { .elements = (ARR).elements, .count = (ARR).count, .capacity = (ARR).capacity }
#define ct_def(TYPE) arrTypeDef(TYPE); mapTypeDef(TYPE)

#define arrDef(TYPE) struct {TYPE* elements; u32 count; u32 capacity;}
#define arrTypeDef(TYPE) typedef struct TYPE##Array {TYPE* elements; u32 count; u32 capacity;} TYPE##Array; typedef struct TYPE##ArrayIterator {u64 idx; TYPE* element;} TYPE##ArrayIterator
#define arrVarDef(TYPE) TYPE##Array  
#define mapTypeDef(TYPE) typedef struct TYPE##Map { ct_Map map; struct { TYPE* elements; u32 count; u32 capacity; } values; } TYPE##Map; typedef struct TYPE##MapIterator {u64 idx; u64 keyHash; TYPE* value;} TYPE##MapIterator
#define mapVarDef(TYPE) TYPE##Map
typedef struct ct_Map {
    u32 count;
} ct_Map;

ct_def(u8);
ct_def(u16);
ct_def(u32);
ct_def(u64);

ct_def(i8);
ct_def(i16);
ct_def(i32);
ct_def(i64);

ct_def(bx);
ct_def(b8);
ct_def(b32);

ct_def(Str8);
ct_def(S16);

ct_def(Vec2);
ct_def(Vec3);
ct_def(Vec4);
ct_def(Quat);
ct_def(Mat4);
ct_def(Mat43);

#endif // _BASE_TYPES_