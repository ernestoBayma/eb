#pragma once

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "core_defs.h"

#if defined(__linux__)
#define OS_LINUX 1
#elif defined(_WIN32) || defined(_WIN64)
#define OS_WINDOWS 1
#elif defined(__APPLE__)
#define OS_MACOS 1
#endif

#if defined(__GNUC__)
  #define COMPILER_GCC 1
#elif defined(__clang__)
  #define COMPILER_CLANG 1
#elif defined(_MSC_VER)
  #define COMPILER_MS 1
#else
  #define COMPILER_OTHER 1
#endif

#define priv_func static
#define global_var static 
#define local_persist static
#define false (bool)0
#define true  (bool)1

#define Assert(expr) assert((expr))

#define Cast(T,expr) (T)(expr)
#define Bytes(n)     Cast(size_t, (n))
#define KB(n)        (Cast(size_t, Bytes(n)) <<  10)
#define MB(n)        (Cast(size_t, Bytes(n)) <<  20)
#define GB(n)        (Cast(size_t, Bytes(n)) <<  30)
#define TB(n)        (Cast(size_t, Bytes(n)) <<  40)

#if __cplusplus
  #define Lang_CPP 1
#endif

#define C_Linkage_Start extern "C" {
#define C_Linkage extern "C"
#define C_Linkage_End }

#if Lang_CPP
  C_Linkage_Start
#endif


#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
  #define INLINE __attribute__((always_inline))
  #if EB_HAS_SSE
    #define TARGET_SSSE3 __attribute__((target("ssse3")))
  #else
    #define TARGET_SSSE3 
  #endif
#elif COMPILER_MS
  #define INLINE __forceinline
  #define TARGET_SSSE3 
#else
  #define INLINE 
  #define TARGET_SSSE3 
#endif

#if COMPILER_MS
# define AlignOf(T) __alignof(T)
#elif COMPILER_CLANG
# define AlignOf(T) __alignof(T)
#elif COMPILER_GCC
# define AlignOf(T) __alignof__(T)
#else
# error AlignOf not defined for this compiler.
#endif

#define StaticArraySize(elem) (sizeof(elem)/sizeof(elem)[0])

#define MemoryCopy(d,s,sz) memmove((d),(s),(sz))
#define MemorySet(d,v,sz) memset((d),(v),(sz))
#define MemoryCompare(a,b,sz) memcmp((a),(b),(sz))
#define MemoryMatch(a,b,sz) (MemoryCompare((a),(b),(sz)) == 0)

#define MemoryZero(m,sz) MemorySet((m), 0, (sz))
#define MemoryZeroStruct(s) MemoryZero((s), sizeof(*(s)))
#define MemoryZeroStaticArray(a) MemoryZero((a), sizeof((a)))
#define StringLength(s) (s) ? strlen((s)) : 0

#define cstr_len_check(cstr) cstr ? cstr_len(cstr) : 0

#define EB_LISTEN_BACKLOG 50
#define EB_SSE_BOUNDARY_BYTES 16

/* C11 defines static_assert to be a macro which calls _Static_assert. */
#if defined(static_assert)
#define EB_STATIC_ASSERT(expr) static_assert(expr, #expr)
#else
#define EB_STATIC_ASSERT(expr)
#endif

#if defined(__GNUC__) || defined(__clang__)
  #define Expect(e,v) __builtin_expect((e), (v))
#else
  #define Expect(e,v) (expr)
#endif

#define Likely(expr) Expect(expr, 1)
#define Unlikely(expr) Expect(expr, 0)


#if defined(OS_LINUX)
#include <linux/limits.h>
#define EB_MAX_PATH_SIZE PATH_MAX
#else
// NOTE(ern): On windows it seems like the path can be 32,767 characters, 
// because the "\\?\" prefix may be expanded to a longer string by the system at run time, 
// and this expansion applies to the total length. And is UTF-16 but right now let's ignore it.
#define EB_MAX_PATH_SIZE 4096
#endif

#ifndef EB_CACHE_LINE
#define EB_CACHE_LINE Bytes(64)
#endif

#define Seconds(n) (n)
#define Seconds_Nano(n) 1000 * Seconds_Mili(n)
#define Seconds_Mili(n) 1000 * Seconds(n)
#define Seconds_Micro(n) 1000 * Seconds_Mili(n)
#define Seconds_Minutes(n) (n) * Seconds(60)
#define Seconds_Hours(n) (n) * Seconds_Minutes(60)

#define Swap(T,a,b) do { T macro_var(temp) = a; a = b; b = macro_var(temp); } while(0)

#define AlignPow2(x,b)     (((x) + (b) - 1) & (~((b) - 1)))
#define AlignDownPow2(x,b) ((x) &  (~((b) - 1)))
#define AlignPadPow2(x,b)  ((0  - (x)) & ((b) - 1))

#define Min(A,B) (((A)<(B))?(A):(B))
#define Max(A,B) (((A)>(B))?(A):(B))
#define ClampTop(A,X) Min(A,X)
#define ClampBot(X,B) Max(X,B)
#define Clamp(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))
#define ExtractBit(word, idx) (((word) >> (idx)) & 1)

#define CstrToInteger(T, cstr) (cstr) ? (T)*((T*)((cstr) - '0')) : ((T)0)

#if defined(EB_HAS_SSE)
#include <tmmintrin.h>
#include <emmintrin.h> 
  #define m128i __m128i
  #define m128d __m128d
  #define m128h __m128h
  #define m128f __m128
#endif
#if defined(EB_HAS_AVX)
#include <immintrin.h> 
  #define m256i __m256i
  #define m256d __m256d
  #define m256h __m256h
  #define m256f __m256
#endif

// Note(ern): Code by Ryan Fleury
//~ rjf: Linked List Building Macros
//- rjf: linked list macro helpers
#define CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define SetNil(nil,p) ((p) = nil)

//- rjf: doubly-linked-lists
#define DLLInsert_NPZ(nil,f,l,p,n,next,prev) (CheckNil(nil,f) ? \
((f) = (l) = (n), SetNil(nil,(n)->next), SetNil(nil,(n)->prev)) :\
CheckNil(nil,p) ? \
((n)->next = (f), (f)->prev = (n), (f) = (n), SetNil(nil,(n)->prev)) :\
((p)==(l)) ? \
((l)->next = (n), (n)->prev = (l), (l) = (n), SetNil(nil, (n)->next)) :\
(((!CheckNil(nil,p) && CheckNil(nil,(p)->next)) ? (0) : ((p)->next->prev= (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define DLLPushBack_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define DLLPushFront_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define DLLRemove_NPZ(nil,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
((n) == (l) ? (l) = (l)->prev : (0)),\
(CheckNil(nil,(n)->prev) ? (0) :\
((n)->prev->next = (n)->next)),\
(CheckNil(nil,(n)->next) ? (0) :\
((n)->next->prev = (n)->prev)))

//- rjf: singly-linked, doubly-headed lists (queues)
#define SLLQueuePush_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
((f)=(l)=(n),SetNil(nil,(n)->next)):\
((l)->next=(n),(l)=(n),SetNil(nil,(n)->next)))
#define SLLQueuePushFront_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
((f)=(l)=(n),SetNil(nil,(n)->next)):\
((n)->next=(f),(f)=(n)))
#define SLLQueuePop_NZ(nil,f,l,next) ((f)==(l)?\
(SetNil(nil,f),SetNil(nil,l)):\
((f)=(f)->next))

//- rjf: singly-linked, singly-headed lists (stacks)
#define SLLStackPush_N(f,n,next) ((n)->next=(f), (f)=(n))
#define SLLStackPop_N(f,next) ((f)=(f)->next)

//- rjf: doubly-linked-list helpers
#define DLLInsert_NP(f,l,p,n,next,prev) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack_NP(f,l,n,next,prev) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront_NP(f,l,n,next,prev) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove_NP(f,l,n,next,prev) DLLRemove_NPZ(0,f,l,n,next,prev)
#define DLLInsert(f,l,p,n) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack(f,l,n) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront(f,l,n) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove(f,l,n) DLLRemove_NPZ(0,f,l,n,next,prev)

//- rjf: singly-linked, doubly-headed list helpers
#define SLLQueuePush_N(f,l,n,next) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront_N(f,l,n,next) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop_N(f,l,next) SLLQueuePop_NZ(0,f,l,next)
#define SLLQueuePush(f,l,n) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront(f,l,n) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop(f,l) SLLQueuePop_NZ(0,f,l,next)

//- rjf: singly-linked, singly-headed list helpers
#define SLLStackPush(f,n) SLLStackPush_N(f,n,next)
#define SLLStackPop(f) SLLStackPop_N(f,next)

#if defined(OS_WINDOWS) && defined(COMPILER_MS)
#include <intrin.h>
#define rdtsc __rdtsc
#else
#include <x86intrin.h>
#define rdtsc __rdtsc
#endif

#define xstringfy(token) #token
#define stringfy(token) xstringfy(token)
#define concat_(a, b) a##x
#define concat(a,b) concat_(a,b)
#define macro_var(var) concat(var,__LINE__)

#define DeferLoop(begin, end) for(int macro_var(_i_) = ((begin), 0); !macro_var(_i_); macro_var(_i_) += 1, (end))
#define DeferLoopChecked(begin, end) for(int macro_var(_i_) = 2 * !(begin); (macro_var(_i_) == 2 ? ((end), 0) : !macro_var(_i_)); macro_var(_i_) += 1, (end))

#define Scope(end) DeferLoopChecked(0, end) 

#define TimedScope(code, var) do {\
  s64 macro_var(_s) = rdtsc();\
  {code}\
  var = rdtsc() - macro_var(_s);\
}while(0)

#define log(msg, args...) do{\
  char macro_var(buff)[1024];\
  time_t macro_var(now) = time(NULL);\
  strftime(macro_var(buff),sizeof(macro_var(buff)),"[%Y-%m-%d %H:%M:%S] ", localtime(&macro_var(now)));\
  fprintf(stderr, "%s", macro_var(buff));\
  fprintf(stderr, (msg), ##args);\
} while(0)

#define PrintVariableString(value) log("[DEBUG] %s = %s\n", stringfy(value), (value));

#ifndef _RELEASE_BUILD
  #define debug() log("%s:%d\n", __func__, __LINE__)
#else
  #define debug()
#endif

#if defined(__has_feature)
	#if __has_feature(address_sanitizer) 
		#define __SANITIZE_ADDRESS__  //For Clang
	#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
  extern void __asan_poison_memory_region(void const volatile *addr, size_t size);
  extern void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
  #define ASAN_POISON_MEMORY_REGION(addr, size) __asan_poison_memory_region((addr), (size))
  #define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
  #define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
  #define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#endif 

#if Lang_CPP
  C_Linkage_End
#endif
