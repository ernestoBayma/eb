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
      arena->prev         = 0;
      arena->current      = arena;
      arena->base_offset  = 0;
      arena->curr_offset  = sizeof(ArenaAllocator);
      arena->options      = options;
      arena->commited     = commited;
      arena->reserved     = reserved;
      arena->grow         = 1;

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
      memory = 0; osMemoryRelease(memory, reserved);
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

static void *arenaPush__impl(ArenaAllocator *arena, size_t size, u64 align)
{
ArenaAllocator *current, *new_block;
u64            pos_mem, pos_new;

      current  = arena->current;
      if(arena->grow) {
          pos_mem = AlignPow2(current->curr_offset, align);
          pos_new = pos_mem + size;

          if(current->reserved < pos_new) {
            u64 reserved_size = arena->options->reserved_size;
            u64 commited_size = arena->options->commited_size;

            if(pos_new > commited_size) {
              reserved_size = pos_new + ARENA_ALLOCATOR_HDR_SZ;
              commited_size = pos_new + ARENA_ALLOCATOR_HDR_SZ;
            }

            new_block = arenaNew(.reserved_size=reserved_size,.commited_size=commited_size);
            if(new_block) {
              new_block->base_offset = current->base_offset + current->reserved;
              SLLStackPush_N(arena->current, new_block, prev);
              current = new_block;

              pos_mem = AlignPow2(current->curr_offset, align);
              pos_new = pos_new + size;
            }
          }
      }

      u64 result_pos = AlignPow2(current->curr_offset, align);
      u64 next_pos   = result_pos + size;

      if(next_pos <= current->reserved) {
        if(next_pos > current->commited) {
          u64 commited_new_aligned, commited_new_clamped, commited_new_size;
          bool   commited_ok;

          commited_new_aligned = AlignPow2(next_pos, current->options->commited_size);
          commited_new_clamped = ClampTop(commited_new_aligned, current->reserved);
          commited_new_size    = commited_new_clamped - current->commited;
          commited_ok          = osMemoryCommit((u8*)current + current->commited, commited_new_size);

          if(commited_ok) 
            current->commited = commited_new_clamped; 
        }
      }

      void *memory = 0;
      if(current->commited >= next_pos) {
          memory = (u8*)current + result_pos;
          current->curr_offset = next_pos;
          ASAN_UNPOISON_MEMORY_REGION((u8*)memory, size);
      }
      return memory;
}

void arenaDebugFmtPrint(ArenaAllocator *a, u8 *buffer, size_t buffer_sz)
{
    snprintf(buffer, 
            buffer_sz, 
            STRFMT":Allocated %llu|Reserved %llu|Commited %llu", 
            STR_PRINT_ARGS(a->options->allocator_name), a->current->curr_offset, a->current->reserved, a->current->commited);
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
  return arenaPush__impl(a, size, align);
}

void *arenaPush__debug(ArenaAllocator *a, size_t size, u64 align, char *file, char *function, int line)
{
  //u8 debug_buffer[1024] = {0};
  //arenaDebugFmtPrint(a, debug_buffer, sizeof(debug_buffer));
  //fprintf(stderr, "[DEBUG] %s => Called in %s:%d trying to alocated %llu:%llu\n", debug_buffer, function, line, size, align); 
  return arenaPush__impl(a, size, align);
}

void *arenaPushContiguous(ArenaAllocator *a, size_t size, u64 align)
{
    u8 saved = a->grow;
    a->grow = 0;
    void *memory = arenaPush__impl(a, size, align);
    a->grow = saved;
    return memory;
}

void arenaPopTo(ArenaAllocator *a, size_t to_position)
{
u64             clamped_pos, new_pos;
ArenaAllocator  *current, *prev;
    
    current         = a->current;
    u64 total_size  = current->base_offset + current->curr_offset;

    if(to_position < total_size) {
        u64 usable_position = Max(to_position, ARENA_ALLOCATOR_HDR_SZ);
        //Find the next current arena
        for(prev = 0; current->base_offset > usable_position; current = prev) {
            prev = current->prev;
            // Don't use this memory anymore.
            ASAN_POISON_MEMORY_REGION(current, current->reserved);
            osMemoryRelease(current, current->reserved);
        }

        a->current = current;

        u64 new_offset  = usable_position - current->base_offset; // Find the new base_offset
        usable_position = Max(new_offset, ARENA_ALLOCATOR_HDR_SZ);

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

//TODO(ern): Problem, wanting to have a scrach allocator and 
// also allocate something to the main allocator.
// Currently this do not work.
ScrachAllocator scrachNew(ArenaAllocator *a)
{
  ScrachAllocator s = {a, arenaPosition(a)};
  return s;
}

void scrachEnd(ScrachAllocator s)
{
    arenaPopTo(s.arena, s.offset);
}
