#pragma once

#include "core.h"
#include "str.h"

#define ARENA_RESERVE_SIZE MB(2)
#define ARENA_COMMIT_SIZE KB(2)
#define ARENA_DEFAULT_ALIGMENT 8

enum {
  ARENA_GROW=(1<<0),
  ARENA_TRACK_ALLOCATIONS=(1<<1)
};

#define ARENA_DEFAULT_FLAGS ARENA_GROW

typedef struct ArenaAllocatorOptions ArenaAllocatorOptions;
struct ArenaAllocatorOptions 
{
    Str allocator_name;
    u64 commited_size;
    u64 reserved_size;
    u64 flags;
};

typedef struct ArenaAllocator ArenaAllocator;
struct ArenaAllocator 
{
  ArenaAllocator        *current;
  ArenaAllocator        *prev;
  ArenaAllocatorOptions options; 
  u64                   base_offset;
  u64                   curr_offset;
  u64                   commited;
  u64                   reserved;
};

typedef struct ScrachAllocator ScrachAllocator;
struct ScrachAllocator 
{
  ArenaAllocator *arena;
  u64 offset;
};

ArenaAllocator *arenaAlloc(ArenaAllocatorOptions *options);
#define arenaNew(...) arenaAlloc(&(ArenaAllocatorOptions){.reserved_size=ARENA_RESERVE_SIZE,.commited_size=ARENA_COMMIT_SIZE, .flags=ARENA_DEFAULT_FLAGS, __VA_ARGS__})

void arenaRelease(ArenaAllocator *a);
size_t arenaPosition(ArenaAllocator *a);
void *arenaPush(ArenaAllocator *a, size_t size, u64 align);
void *arenaPushContiguous(ArenaAllocator *a, size_t size, u64 align);
void *arenaPush__debug(ArenaAllocator *a, size_t size, u64 align, char *file, char *function, int line);
void arenaPopTo(ArenaAllocator *a, size_t original_pos);
void arenaClear(ArenaAllocator *a);
ArenaAllocator *arenaJoin(ArenaAllocator *a, ArenaAllocator *b);
ScrachAllocator scrachNew(ArenaAllocator *a);
void scrachEnd(ScrachAllocator s);

#define push_arena_array(a, T, c) (T*)arenaPush((a), sizeof(T)*(c), Max(8, AlignOf(T)))
#define push_arena_array_aligned(a, T, c, al) (T*)arenaPush((a), sizeof(T)*(c), (al))
#define push_arena_array_zero(a, T, c) (T*)MemoryZero(push_arena_array(a,T,c), sizeof(T)*(c))
#define push_arena_array_zero_aligned(a, T, c, al) (T*)MemoryZero(push_arena_array_aligned(a,T,c,al), sizeof(T)*(c))
