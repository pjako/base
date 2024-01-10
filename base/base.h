#ifndef _BASE_H_
#define _BASE_H_

#ifdef __cplusplus
#define CPP_ENV 1
#else
#define CPP_ENV 0
#endif


#pragma mark - Compiler

#define COMPILER_MSVC 0
#define COMPILER_CLANG 0
#define COMPILER_GCC 0

#ifdef _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#elif defined(__clang__)
#undef COMPILER_CLANG
#define COMPILER_CLANG 1
#elif defined(__GNUC__)
#undef COMPILER_GCC
#define COMPILER_GCC 1
#endif

////////////////////////////////
// NOTE(pjako): Operation System
#define OS_WIN 0
#define OS_OSX 0
#define OS_IOS 0
#define OS_ANDROID 0
#define OS_LINUX 0
#define OS_UNIX 0
#define OS_EMSCRIPTEN 0

#define KILOBYTE(KIL) (1024LL*(1LL)*(KIL))
#define MEGABYTE(MEG) (1024LL*(KILOBYTE(1LL))*(MEG))
#define GIGABYTE(GIG) (1024LL*(MEGABYTE(1LL))*(GIG))

#ifdef _WIN32
#undef OS_WIN
#define OS_WIN 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#undef OS_OSX
#define OS_OSX 1
#elif defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
#undef OS_IOS
#define OS_IOS 1
#elif __ANDROID__
#undef OS_ANDROID
#define OS_ANDROID 1
#elif __linux__
#undef OS_LINUX
#define OS_LINUX 1
#elif __unix__
#undef OS_UNIX
#define OS_UNIX 1
#elif __EMSCRIPTEN__
#undef OS_EMSCRIPTEN
#define OS_EMSCRIPTEN 1
#else
#error "Unknown Operation System"
#endif
#if OS_OSX || OS_IOS
#define OS_APPLE 1
#else
#define OS_APPLE 0
#endif

////////////////////////////////
// NOTE(pjako): Operation System Name
#if OS_WIN
#define OS_NAME "Windows"
#elif OS_OSX
#define OS_NAME "OSX"
#elif  OS_IOS
#define OS_NAME "IOS"
#elif  OS_ANDROID
#define OS_NAME "Android"
#elif  OS_LINUX
#define OS_NAME "Linux"
#elif  OS_UNIX
#define OS_NAME "Unix"
#elif  OS_EMSCRIPTEN
#define OS_NAME "Emscripten"
#else
#error "Unknown OS"
#endif

////////////////////////////////
// NOTE(pjako): Architecture
#define ARCH_X64 0
#define ARCH_ARM64 0

#if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#undef ARCH_X64
#define ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#undef ARCH_ARM64
#define ARCH_ARM64 1
#else
#error "Unknown CPU architecture"
#endif

#define INLINE static inline
#if COMPILER_MSVC
#define FORCE_INLINE static __forceinline
#else
#define FORCE_INLINE static __attribute__((always_inline))
#endif

#if COMPILER_CLANG || COMPILER_GCC
#define ALIGNED_STRUCT(NAME, ALIGN, CONTENT) typedef struct NAME CONTENT __attribute__ ((aligned (ALIGN))) NAME
#define ALIGNED_STRUCT_START(NAME, ALIGN) typedef struct NAME 
#define ALIGNED_STRUCT_END(NAME, ALIGN) __attribute__ ((aligned (ALIGN))) NAME
#define ALIGN_DECL(_align, _decl) _decl __attribute__( (aligned(_align) ) )
#define THREAD_LOCAL __thread
#else
#define ALIGNED_STRUCT(NAME, ALIGN, CONTENT) typedef struct __declspec(align(ALIGN)) NAME CONTENT NAME
#define ALIGNED_STRUCT_START(NAME, ALIGN) typedef struct __declspec(align(ALIGN)) NAME 
#define ALIGNED_STRUCT_END(NAME, ALIGN) NAME
#define ALIGN_DECL(_align, _decl) __declspec(align(_align) ) _decl
#define THREAD_LOCAL __declspec(thread)
#endif

#if __cplusplus >= 201103
#define DEFINE_ALIGNED(def, a) alignas(a) def
#else
#if defined(_WINDOWS) || defined(XBOX)
#define DEFINE_ALIGNED(def, a) __declspec(align(a)) def
#elif defined(__APPLE__)
#define DEFINE_ALIGNED(def, a) def __attribute__((aligned(a)))
#else
//If we haven't specified the platform here, we fallback on the C++11 and C11 keyword for aligning
//Best case -> No platform specific align defined -> use this one that does the same thing
//Worst case -> No platform specific align defined -> this one also doesn't work and fails to compile -> add a platform specific one :)
#define DEFINE_ALIGNED(def, a) alignas(a) def
#endif
#endif

#define COMPILER_ASSERT(exp)  typedef char __compilerAssert##__LINE__[(exp) ? 1 : -1]


#pragma mark - API

#ifndef API
#define API extern
#endif

#ifndef LOCAL
#define LOCAL static
#endif


#ifndef DLL_API
#if OS_WIN
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif
#endif


////////////////////////////////
// NOTE(pjako): Linked List
// dll == double linked list
// sll == single linked list

#define dll_pushBackNp(f,l,n, next, prev) ((f)==0?\
                                          ((f)=(l)=(n),(n)->next=(n)->prev=0):\
                                          ((n)->prev=(l),(l)->next=(n),(l)=(n),(n)->next = 0))
#define dll_removeNp(f,l,n, next, prev) (((f)==(n)?\
                                          ((f)=(f)->next,(f)->prev=0):\
                                          (l)==(n)?\
                                          ((l)=(n)->prev,(l)->next=0):\
                                          ((n)->next->prev=(n)->prev,\
                                           (n)->prev->next=(n)->next)))
#define dll_pushBack(f,l,n) dll_pushBackNp(f,l,n, next, prev)
#define dll_pushFront(f,l,n) dll_pushBackNp(f,l,n, prev, next)
#define dll_remove(f,l,n, next, prev) dll_removeNp(f,l,n, next, prev)

#define sll_queuePushN(f,l,n,next) ((f)==0?\
                                    (f)=(l)=(n):\
                                    ((l)->next=(n),(l)=(n)),\
                                    (n)->next=0)
#define sll_queuePush(f,l,n,next) sll_queuePushN(f,l,n,next)

#define sll_queuePushFrontN(f,l,n,next) ((f)==0?\
                                         ((f)=(l)=(n),(n)->next=0):\
                                         ((n)->next=(f),(f)=(n)))
#define sll_queuePushFront(f,l,n,next) sll_queuePushFrontN(f,l,n,next)

#define sll_queuePopN(f,l,next) ((f)=(l)?\
                                 (f)=(l)=0:\
                                 (f)=(f)->next)
#define sll_queuePop(f,l,next) sll_queuePopN(f,l,next)

#define sll_stackPushN(f,n,next) ((f)==0?\
                                 (f)=(l)=0:\
                                 ((n)->next=(f), (f)=(n)))
#define sll_stackPush(f,n,next) sll_stackPushN(f,n,next)

#define sll_stackPopN(f,next) ((f)==0?0:\
                               (f)=(f)->next)
#define sll_stackPop(f,next) sll_stackPopN(f,next)

#define unused(x) (void)(x)
// dont use this with VLA args, could have side effects
#define unusedVars(...) sizeof(__VA_ARGS__)

#define minVal(a, b) ((a) < (b) ? (a) : (b))
#define maxVal(a, b) ((a) > (b) ? (a) : (b))
#define clampVal(MINVAL, MAXVAL, VALUE) (maxVal(minVal(MAXVAL, VALUE), MINVAL))
#define absVal(A)    ((A) < 0 ? -(A) : (A))
#define squareVal(A) ((A) * (A))


#define alignDown(PTR, ALIGN)  ((umm_cast(PTR)) & ~(i32_cast(ALIGN) - 1))
#define alignUp(PTR, ALIGN)   (((umm_cast(PTR)) +  (i32_cast(ALIGN) - 1)) & ~(i32_cast(ALIGN) - 1))
#define isAligned(PTR, ALIGN)  ((umm_cast(PTR)  &  (i32_cast(ALIGN) - 1)) == 0)

#define OS_MEM_ALIGNMENT (8)
#define os_alignDown(PTR) alignDown((PTR), OS_MEM_ALIGNMENT)
#define os_alignUp(PTR)  alignUp((PTR), OS_MEM_ALIGNMENT)
#define os_isAligned(PTR)isAligned((PTR), OS_MEM_ALIGNMENT)

#define OS_DEFAULT_PAGE_SIZE (1024 * 1024 * 4)

#define clampTop(V, T) ((V > T) ? T : V)
#define ceilVal(a, b) (((a) / (b)) + (((a) % (b)) > 0 ? 1 : 0))
#define roundVal(a, b) (((a) / (b)) + (((a) % (b)) >= 5 ? 1 : 0))

#define isPowerOf2(x) (((x) != 0u) && ((x) & ((x) - 1)) == 0u)

#define u32_nextPowerOfTwo(VAL) ASSERT(VAL > 0); VAL--; VAL |= VAL >> 1; VAL |= VAL >> 2; VAL |= VAL >> 4; VAL |= VAL >> 8; VAL |= VAL >> 16; VAL += 1

#define sizeOf(S) ((i64)sizeof(S))
#define countOf(V) (sizeOf(V) / sizeOf((V)[0]))


#if defined(_Typeof)
// C23 typeof
#define typeOf(TYPE) _Typeof(TYPE)
#elif defined(typeof)
// non standard typeof in GCC/Clang
#define typeOf(TYPE) typeof(TYPE)
#elif defined(__typeof__)
#define typeOf(TYPE) __typeof__(TYPE)
#elif __cpp_decltype >= 200707L
// decltype from C++ could be a bit problematic since there are some subtle differences to typeof/_Typeof
#define typeOf(TYPE) decltype(TYPE)
#else
#define typeOf(TYPE) void*
//#error "No typeof variant exist in this enviroment"
#endif

#if CPP_ENV
#define CPLiteral(Type, ...) { __VA_ARGS__ }
#else
#define CPLiteral(Type, ...) (TYPE) { __VA_ARGS__ }
#endif

#if CPP_ENV
#define ZeroStruct() {}
#else
#define ZeroStruct() {0}
#endif

//
// Debug
//

#if COMPILER_MSVC
#define ASSERT(C) while(!(C)) __assume(0)
#else
#include "assert.h"
#define ASSERT(C) assert(C)
// while(!(C)) __builtin_unreachable()
#endif

#define BASE_STRING_JOIN_IMMEDIATE(ARG1, ARG2) ARG1 ## ARG2
#define BASE_STRING_JOIN_DELAY(ARG1, ARG2) BASE_STRING_JOIN_IMMEDIATE(ARG1, ARG2)
#define BASE_STRING_JOIN(ARG1, ARG2) BASE_STRING_JOIN_DELAY(ARG1, ARG2)

#ifdef COMPILER_MSVC
  #define BASE_UNIQUE_NAME(NAME) BASE_STRING_JOIN(NAME,__COUNTER__)
#else
  #define BASE_UNIQUE_NAME(NAME) BASE_STRING_JOIN(NAME,__LINE__)
#endif

#ifndef STATIC_ASSERT
  #define STATIC_ASSERT(EXP) typedef u8 BASE_UNIQUE_NAME(__staticAssertDummyArr)[(EXP)?1:-1]
#endif

#if COMPILER_MSVC
#define debugBreak() __debugbreak()
#elif COMPILER_CLANG
#define debugBreak() __builtin_debugtrap()
#elif COMPILER_GCC
#define debugBreak() __builtin_trap()
#else // cross platform implementation
#define debugBreak() for (;;) {int* int3 = (int*)3L; *int3 = 3; break;}
//#define DEBUG_BREAK() (*((void*) 0) = 0)
#endif

#endif /* _BASE_H_ */