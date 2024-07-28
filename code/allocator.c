/*
 *
 *  Copyright (C) 2024 Ernesto Bayma
 * - Need to fix:
 *  There is some pesky memory leak going on.
 *
 */

#include "core.h"
#include "util.h"
#include "allocator.h"
#include "os.h"

priv_func 
void *arenaAlloc__impl(u64 commit_size, u64 reserved_size, 
                       u64 *out_commit, u64 *out_reserved);

ArenaAllocator *arenaAlloc(ArenaAllocatorOptions *options)
{
void           *memory;
ArenaAllocator *arena;
u64             commited, reserved;

    commited = reserved = 0;
    memory = arenaAlloc__impl(options->commited_size, 
                              options->reserved_size, 
                             &commited, &reserved);
    arena = (ArenaAllocator*)memory;
    if(arena) {
      arena->prev               = 0;
      arena->current            = arena;
      arena->base_offset        = 0; 
      arena->curr_offset        = sizeof(ArenaAllocator); 
      arena->commited           = commited;
      arena->reserved           = reserved;

      MemoryCopy(&arena->options, options, sizeof(ArenaAllocatorOptions));
      ASAN_POISON_MEMORY_REGION(memory, commited);
      ASAN_UNPOISON_MEMORY_REGION(memory, sizeof(ArenaAllocator));
  }
  return arena;
}

priv_func void *arenaAlloc__impl(u64 commit_size, u64 reserved_size, 
                                 u64 *out_commit, u64 *out_reserved)
{
void *memory;
size_t page_size;
u64    commited, reserved;

  page_size = osPageSize();
  commited  = AlignPow2(commit_size, page_size);
  reserved  = AlignPow2(reserved_size, page_size);
  memory    = osMemoryReserve(reserved);

  if(!osMemoryCommit(memory, commited)) {
      osMemoryRelease(memory, reserved);
      memory = 0;
      commited = reserved = 0;
  }

  *out_commit     = commited;
  *out_reserved   = reserved;

  return memory;
}

void arenaRelease(ArenaAllocator *a)
{
  for(ArenaAllocator *curr = a->current, *prev = 0; curr != 0; curr = prev) {
      prev = curr->prev;
      osMemoryRelease(curr, curr->reserved);
  }
}

C_Inline void arenaPrintAllocationInfo(ArenaAllocator *arena, u64 position_aligned, u64 added_position, size_t size, u64 align)
{
  log("[DEBUG] Tracked allocation ====================\n");
  log("[DEBUG] Name:'"STRFMT"'\n", STR_PRINT_ARGS(arena->options.allocator_name));
  log("[DEBUG] New allocation = %lu bytes\n", size);
  log("[DEBUG] Memory Aligment: %llu bytes\n", align);
  PrintVariableU64(arena->options.reserved_size);
  PrintVariableU64(arena->options.commited_size);
  PrintVariableU64(arena->curr_offset);
  PrintVariableU64(arena->reserved);
  PrintVariableU64(arena->commited);
  PrintVariableU64(position_aligned);
  PrintVariableU64(added_position);
  log("[DEBUG] End tracking ==========================\n");
}

static void *arenaPush__impl(ArenaAllocator *arena, size_t size, u64 align)
{
ArenaAllocator *current, *new_block;
u64            pos_mem, pos_new;

      current  = arena->current;
      pos_mem = AlignPow2(current->curr_offset, align);
      pos_new = pos_mem + size;

      if(arena->options.flags & ARENA_GROW)
      {
          if(current->commited <= pos_new)
          {
            u64 reserved_size = arena->options.reserved_size;
            u64 commited_size = arena->options.commited_size;
            if((pos_new + sizeof(ArenaAllocator)) > reserved_size)
            {
              reserved_size = pos_new + sizeof(ArenaAllocator);
              commited_size = pos_new + sizeof(ArenaAllocator);
            }

            new_block = arenaNew(
                .reserved_size=reserved_size,
                .commited_size=commited_size,
                .flags=current->options.flags);

            if(new_block) 
            {
              new_block->base_offset = current->base_offset + current->reserved;
              SLLStackPush_N(arena->current, new_block, prev);
              current = new_block;
              pos_mem = AlignPow2(current->curr_offset, align);
              pos_new = pos_mem + size;
            }
          }
      }

      if(arena->options.flags & ARENA_TRACK_ALLOCATIONS) 
      {
          arenaPrintAllocationInfo(current, pos_mem, pos_new, size, align); 
      }

      u64 result_pos = AlignPow2(current->curr_offset, align);
      u64 next_pos   = result_pos + size;

      if(next_pos > current->commited)
      {
        u64 commited_new_aligned,commited_new_clamped,commited_new_size;
        commited_new_aligned = AlignPow2(next_pos, current->options.commited_size);
        commited_new_clamped = ClampTop(commited_new_aligned, current->reserved);
        commited_new_size    = commited_new_clamped - current->commited;

        if(osMemoryCommit((u8*)current + current->commited, commited_new_size)) 
        {
            current->commited = commited_new_clamped; 
        }
      }

      void *memory = 0;
      if(current->commited > next_pos)
      {
          memory = (u8*)current + result_pos;
          current->curr_offset = next_pos;
          ASAN_UNPOISON_MEMORY_REGION((u8*)memory, size);
      }

      return memory;
}

ArenaAllocator *arenaJoin(ArenaAllocator *a, ArenaAllocator *b)
{
ArenaAllocator *arena;
u64             base_ajust;

    arena       = a->current;
    base_ajust  = arena->base_offset + arena->reserved; 
    for(ArenaAllocator *node = b->current; node; node = node->prev) {
      node->base_offset += base_ajust;
    }
    b->prev = arena->current;
    arena->current = b->current;
    b->current = b;
    return b;
}

size_t arenaPosition(ArenaAllocator *a)
{
    return a->current->base_offset + a->current->curr_offset;
}

void *arenaPush(ArenaAllocator *a, size_t size, u64 align)
{
void *ret;

  ret = arenaPush__impl(a, size, align);
  if(!ret) {
      fprintf(stderr, "[DEBUG] Allocation failed\n");
  }
  return ret;
}

void *arenaPush__debug(ArenaAllocator *a, size_t size, u64 align, char *file, char *function, int line)
{
  return arenaPush__impl(a, size, align);
}

void *arenaPushContiguous(ArenaAllocator *a, size_t size, u64 align)
{
    u32 saved = a->options.flags & ARENA_GROW;
    a->options.flags &= ~(ARENA_GROW);
    void *memory = arenaPush__impl(a, size, align);
    a->options.flags |= saved;
    return memory;
}

void arenaPopTo(ArenaAllocator *a, size_t to_position)
{
u64             clamped_pos, new_pos;
ArenaAllocator  *current, *prev;
    
    current         = a->current;
    u64 total_size  = current->base_offset + current->curr_offset;

    if(to_position < total_size) {
        u64 usable_position = Max(to_position, sizeof(ArenaAllocator));
        //Find the next current arena
        for(prev = 0; current->base_offset >= usable_position; current = prev) {
            prev = current->prev;
            osMemoryRelease(current, current->reserved);
        }

        a->current = current;
        u64 new_offset  = usable_position - current->base_offset; // Find the new base_offset
        usable_position = Max(new_offset, sizeof(ArenaAllocator));

        Assert(new_offset <= current->curr_offset);
        Assert(usable_position <= current->curr_offset);

        ASAN_POISON_MEMORY_REGION((u8*)current+usable_position, (current->curr_offset - usable_position));
        current->curr_offset = usable_position;
    }
}

void arenaClear(ArenaAllocator *a)
{
  arenaPopTo(a, 0);
}

ScrachAllocator scrachNew(ArenaAllocator *a)
{
  ScrachAllocator s = {a, arenaPosition(a)};
  return s;
}
void scrachEnd(ScrachAllocator s)
{
    arenaPopTo(s.arena, s.offset);
}

