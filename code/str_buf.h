#if !defined(EB_STR_BUF_H)
#define EB_STR_BUF_H

#define EB_USE_MEMCPY 1
#define EB_USE_MEMSET 1

#include <stdarg.h>
#include <stdio.h>

#include "core.h"
#include "allocator.h"
#include "str.h"

#if defined(Lang_CPP)
C_Linkage_Start
#endif

#define STRBUF_ALIGNMENT 16
typedef struct StrBuf StrBuf;

struct __impl_eb_str_buf {
  u64     size;
  u64     cap;
  u8      *ptr;
  u8      *last_char; // Note(ern): Makes trivial to concatenate
};

#define StrBufIncrease    (1 << 0)
#define StrBufSharedArena (1 << 1)

typedef struct StrBufOptions StrBufOptions;
struct StrBufOptions {
   u8                 *backing_buffer;
   u64                 capacity;
   u64                 flags;
   ArenaAllocator     *shared_arena;
};

struct StrBuf 
{
  struct __impl_eb_str_buf *buf;
  StrBufOptions            *opts;
  ArenaAllocator           *string_arena;
};

StrBuf strbuf_allocate(StrBufOptions *options);
#define strbuf_alloc(...) strbuf_allocate(&(StrBufOptions){.flags=StrBufIncrease|StrBufSharedArena, __VA_ARGS__})
#define strbuf_alloc_from_cstr(buf, cstr, ...) do{ \
  buf = strbuf_allocate(&(StrBufOptions){.flags=StrBufIncrease|StrBufClearArena, __VA_ARGS__}); \
  strbuf_new_impl(&buf, cstr, StringLength(cstr)); \
while(0)
#define strbuf_alloc_from_str_(buf, str, ...) do{ \
  buf = strbuf_allocate(&(StrBufOptions){.flags=StrBufIncrease|StrBufClearArena, __VA_ARGS__}); \
  strbuf_new_impl(&buf, STR_DATA(str), STR_SIZE(str)); \
while(0)

bool   strbuf_new_impl(StrBuf *buf, const char *cstr, size_t size);
StrBuf strbuf_new_str(Str str);
StrBuf strbuf_new_cstr(const char *cstr);
void   strbuf_release(StrBuf *buf);
Str    strbuf_get_str(StrBuf *buf);
bool   strbuf_clear(StrBuf *buf);
bool   strbuf_concat_str(StrBuf *buf, Str str, bool *new_allocation);
bool   strbuf_concat_cstr(StrBuf *buf, const char *cstr, bool *new_allocation);
bool   strbuf_concat_sized(StrBuf *buf, const char *cstr, u64 str_sz, bool *new_allocation);
bool   strbuf_concatf(StrBuf *buf, const char *fmt, ...);

#define strbuf_new(c) _Generic((c),\
const char *: strbuf_new_cstr,\
char *:       strbuf_new_cstr,\
Str:          strbuf_new_str\
)(c)

#define strbuf_concat(b,s,n) _Generic((s),\
const char *: strbuf_concat_cstr,\
char *: strbuf_concat_cstr,\
Str:    strbuf_concat_str\
)(b,s,n)

#define STRBUF_EMPTY (StrBuf){0}
#define strbuf_buf_valid(s) (s).buf->ptr != 0

#endif
#if defined(EB_STR_BUF_IMPLEMENTATION)

bool strbuf_concat_impl(StrBuf *buf, const char *cstr, u64 str_sz, bool *new_allocation);
priv_func u8 *strbuf_alloc_impl(StrBuf *buf, size_t capacity);

StrBuf strbuf_new_str(Str str)
{
StrBuf b;

  b = strbuf_alloc();
  strbuf_new_impl(&b, STR_DATA(str), STR_SIZE(str));
  return b;
}
StrBuf strbuf_new_cstr(const char *cstr)
{
StrBuf b;

  b = strbuf_alloc();
  strbuf_new_impl(&b, cstr, StringLength(cstr));
  return b;
}

priv_func
u8 *strbuf_alloc_impl(StrBuf *buf, size_t capacity)
{
u8 *ptr = 0;
  
  if(buf->string_arena) {
      ptr = push_arena_array_zero_aligned(buf->string_arena, u8, capacity, STRBUF_ALIGNMENT);
  } else {
      ArenaAllocator *string_arena;
      string_arena = arenaNew(.commited_size=capacity, 
                              .reserved_size=capacity);

      buf->string_arena = string_arena;
      ptr = push_arena_array_zero_aligned(string_arena, u8, capacity, STRBUF_ALIGNMENT);
  }

  return ptr;
}

StrBuf strbuf_allocate(StrBufOptions *options)
{
StrBufOptions flags;
StrBuf buf = {0};
struct __impl_eb_str_buf *h;
bool   grow = false;
u8    *ptr;
u64   capacity;

      ptr      = 0;
      capacity = Bytes(4096);

      buf.opts = options;
      if(options->capacity > 0) 
          capacity = options->capacity;

      if(options->backing_buffer) {
          ptr             = options->backing_buffer;
          options->flags  = options->flags & ~(StrBufSharedArena);
      }
      if(options->shared_arena) {
        buf.string_arena = options->shared_arena;
        options->flags   = options->flags | StrBufSharedArena;
      }
      
      if(!ptr) {
          ptr = strbuf_alloc_impl(&buf, capacity); 
      }

      if(options->backing_buffer) {
        h = ((struct __impl_eb_str_buf*)ptr);
        capacity -= sizeof(struct __impl_eb_str_buf);
        ptr += sizeof(struct __impl_eb_str_buf);
      } else {
        h = push_arena_array(buf.string_arena, struct __impl_eb_str_buf, 1);
      }

      buf.buf            = h;
      buf.buf->ptr       = ptr;
      buf.buf->cap       = capacity;
      buf.buf->size      = 0;
      buf.buf->last_char = &ptr[0];

      return buf;
}

bool strbuf_new_impl(StrBuf *b, const char *cstr, size_t size)
{
    if(!b || !cstr) 
      return false;

    if(b->buf->cap <= size && !(b->opts->flags & StrBufIncrease)) 
      return false;

    MemoryCopy(b->buf->ptr, cstr, size);
    b->buf->last_char = &b->buf->ptr[size];
    b->buf->size      = size;

    return true;
}

bool strbuf_concat_str(StrBuf *buf, Str str, bool *new_allocation)
{
    return strbuf_concat_impl(buf, STR_DATA(str), STR_SIZE(str), new_allocation);
}

bool strbuf_concat_cstr(StrBuf *buf, const char *cstr, bool *new_allocation)
{
    return strbuf_concat_impl(buf, cstr, StringLength(cstr), new_allocation);
}

bool strbuf_concat_sized(StrBuf *buf, const char *cstr, u64 str_sz, bool *new_allocation)
{
  return strbuf_concat_impl(buf, cstr, str_sz, new_allocation);
}

bool strbuf_concat_impl(StrBuf *buf, const char *cstr, u64 str_sz, bool *new_allocation)
{
  ArenaAllocator *arena;
  u8 *ptr;

  if(!buf || !cstr) 
    return false;

  u64 current_size    = buf->buf->size;
  u64 current_cap     = buf->buf->cap;
  u64 new_concat_size = current_size + str_sz;
  u64 new_cap         = new_concat_size;
  ptr                 = 0;
  
  if(new_concat_size >= current_cap) {
    if(buf->opts->flags & StrBufIncrease) {
        if(new_allocation) 
          *new_allocation = true;

        if(buf->opts->flags & StrBufSharedArena) {
           u8 *ptr = push_arena_array_zero_aligned(buf->string_arena, u8, new_concat_size, STRBUF_ALIGNMENT); 
           if(!ptr)
              return false;
        } else {
          arena        = arenaNew(.commited_size=new_concat_size,.reserved_size=new_cap);
          u8 *ptr      = push_arena_array_zero_aligned(arena, u8, new_concat_size, STRBUF_ALIGNMENT);
          if(!ptr) {
            arenaRelease(arena);
            return false;
          } 
          if(buf->string_arena) arenaRelease(buf->string_arena);
          buf->string_arena = arena;
        }
    } else {
        return false;
    }
  } 

  if(ptr) {
    MemoryCopy(ptr, buf->buf->ptr, buf->buf->size);
    buf->buf->ptr         = ptr;
    buf->buf->last_char   = &ptr[buf->buf->size];
  } 

  MemoryCopy(buf->buf->last_char, cstr, str_sz);
  
  buf->buf->size      += str_sz;
  buf->buf->cap       = new_cap;
  buf->buf->last_char = &buf->buf->ptr[new_concat_size];

  return true;
}

bool strbuf_concatf(StrBuf *buf, const char *fmt, ...)
{
va_list va;
bool    ret;
u8      *dyn, *used_buf;
s32     size;
ScrachAllocator sc;
cacheline_t fixed;
    
    ret = true;

    sc = scrachNew(buf->string_arena);
    va_start(va, fmt);
    size = vsnprintf(fixed.b, sizeof(fixed), fmt, va);
    va_end(va);
    if(size == -1) 
      return false;
    used_buf = fixed.b;
    if(size >= sizeof(fixed)) {
        dyn = (u8*)push_arena_array(sc.arena, char, size + 1);
        if(!dyn) {
          ret = false;
          goto end; 
        }
        used_buf = dyn; 
        used_buf[size] = 0;
        va_start(va, fmt);
        if(vsnprintf(used_buf, size, fmt, va) == -1) {
          ret = false;
          goto end;
        }
        va_end(va);
    }

    ret = strbuf_concat_sized(buf, used_buf, size, 0);
end:
    scrachEnd(sc);
    return ret;
}

bool strbuf_clear(StrBuf *buf)
{
u8 *ptr;

  if(!buf || !buf->buf || !buf->buf->ptr)
    return false;

  ptr = buf->buf->ptr;
  MemoryZero(ptr, buf->buf->cap);
  buf->buf->size = 0;
  buf->buf->last_char = &ptr[buf->buf->size];

  return true; 
}

Str strbuf_get_str(StrBuf *buf)
{
Str ret;

  ret = buf ? cstr_sized(buf->buf->ptr, buf->buf->size) : STR_INVALID;
  return ret;
}

void strbuf_release(StrBuf *buf)
{
  if(!buf)
    return;
  if(buf->opts->flags & StrBufSharedArena)
    return;
  arenaRelease(buf->string_arena);
}
#endif

#if defined(Lang_CPP)
C_Linkage_End
#endif

