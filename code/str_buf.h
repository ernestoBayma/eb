#if !defined(EB_STR_BUF_H)
#define EB_STR_BUF_H

#define EB_USE_MEMCPY 1
#define EB_USE_MEMSET 1

#include <stdarg.h>
#include <stdio.h>

#include "core.h"
#include "allocator.h"
#include "str.h"

#if __cplusplus
extern "C" {
#endif

#define STRBUF_ALIGNMENT 16
typedef struct StrBuf StrBuf;

/*  NOTE/TODO(ern):
    I need to decide what is the format to this String buffer struct, right now the fixed buffer takes alot of space, and
    i'm not sure if there any gain to it. Using a union is also debatable, even a bad design, need to test it more.

    Allocation is using the arena allocator, as a contiguous block for only that string. Also a debatable choice, as each
    string buffer now has a lifetime instead of the arena lifetime deciding when that allocation finishes.

    Improviments are needed.
*/
struct __impl_eb_str_buf_header {
  u64     size;
  u64     cap;
  u8      *ptr;
  u8      *last_char; // Note(ern): Makes trivial to concatenate
};
struct __impl_eb_str_fixed {
  u64 size; 
  u64 cap;
  cacheline_t fixed_buf;
  u8          *last_char; // Note(ern): Makes trivial to concatenate
};



struct StrBuf {
  struct __impl_eb_str_fixed fixed_buf;
  struct __impl_eb_str_buf_header dyn_buf;
  bool dynamic;
  ArenaAllocator *string_arena;
};

StrBuf strbuf_empty(void);
StrBuf strbuf_new_impl(const char *cstr, size_t size);
StrBuf strbuf_new_str(Str str);
StrBuf strbuf_new_cstr(const char *cstr);
void strbuf_release(StrBuf *buf);
Str strbuf_get_str(StrBuf *buf);
bool strbuf_clear(StrBuf *buf);
bool strbuf_concat_str(StrBuf *buf, Str str);
bool strbuf_concat_cstr(StrBuf *buf, const char *cstr);
StrBuf strbuf_new_sized(size_t size);
bool strbuf_concat_sized(StrBuf *buf, const char *cstr, s32 str_sz);
bool strbuf_concatf(StrBuf *buf, const char *fmt, ...);

#define strbuf_new(c) _Generic((c),\
const char *: strbuf_new_cstr,\
char *:       strbuf_new_cstr,\
Str:          strbuf_new_str\
)(c)

#define strbuf_concat(b,s) _Generic((s),\
const char *: strbuf_concat_cstr,\
char *: strbuf_concat_cstr,\
Str:    strbuf_concat_str\
)(b,s)

#define STRBUF_EMPTY (StrBuf){0}

#define strbuf_buf_valid(s) ((s).dynamic) ? (s).dyn_buf.ptr != 0 : true
#endif
#if defined(EB_STR_BUF_IMPLEMENTATION)

bool strbuf_concat_impl(StrBuf *buf, const char *cstr, s32 str_sz);
#define NEED_TO_ALLOCATE(s, needed) ((s).fixed_buf.size + needed + 1) >= (s).fixed_buf.cap

StrBuf strbuf_empty(void) 
{
StrBuf buf = {0};

  buf.fixed_buf.cap = sizeof(buf.fixed_buf.fixed_buf.b);
  return buf;
}

StrBuf strbuf_new_str(Str str)
{
  return strbuf_new_impl(STR_DATA(str), STR_SIZE(str));
}
StrBuf strbuf_new_cstr(const char *cstr)
{
  return strbuf_new_impl(cstr, StringLength(cstr));
}

StrBuf strbuf_new_sized(size_t size)
{
StrBuf buf;

    buf = strbuf_empty();
    if((size + 1) > buf.fixed_buf.cap) {
        ArenaAllocator *string_arena;

        size_t used_size      = size + 1;
        size_t capacity       = Max(used_size, MB(4));

        string_arena          = arenaNew(.commited_size=size, 
                                         .reserved_size=capacity);
        buf.string_arena      = string_arena;

        u8 *ptr               = push_arena_array_zero_aligned(string_arena, u8, used_size, STRBUF_ALIGNMENT);
        buf.dyn_buf.ptr       = ptr;
        buf.dyn_buf.cap       = capacity;
        buf.dyn_buf.size      = 0;
        buf.dynamic           = true;
        buf.dyn_buf.last_char = &buf.dyn_buf.ptr[0];
    }
    
    return buf;
}

StrBuf strbuf_new_impl(const char *cstr, size_t size)
{
StrBuf buf;
u8**    used_internal_buf;
u8**    used_last_char;

    if(!cstr) {
      return STRBUF_EMPTY;
    }

    buf = strbuf_new_sized(size); 
    if(buf.dynamic) {
        *used_internal_buf = buf.dyn_buf.ptr;
        *used_last_char    = buf.dyn_buf.last_char;
        buf.dyn_buf.size   = size;
    } else {
        *used_internal_buf   = buf.fixed_buf.fixed_buf.b;
        *used_last_char      = buf.fixed_buf.last_char;
        buf.fixed_buf.size  = size;
    }

    MemoryCopy(*used_internal_buf, cstr, size);
    *used_last_char = used_internal_buf[size];

    return buf;
}

priv_func
u8 *strbuf_alloc(StrBuf *buf, size_t size, size_t capacity)
{
u8 *ptr = 0;
  
  size_t used_size = size + 1;
  if(buf->string_arena) {
      ptr = push_arena_array_zero_aligned(buf->string_arena, u8, used_size, STRBUF_ALIGNMENT);
  } else {
      ArenaAllocator *string_arena;
      string_arena = arenaNew(.commited_size=size, 
                                .reserved_size=capacity);

      buf->string_arena = string_arena;
      ptr = push_arena_array_zero_aligned(string_arena, u8, used_size, STRBUF_ALIGNMENT);
  }

  buf->dyn_buf.ptr  = ptr;
  buf->dynamic      = true;
  buf->dyn_buf.cap  = capacity;
  buf->dyn_buf.size = size;
  return ptr;
}

StrBuf strbuf_new_impl_str(Str str)
{
  return strbuf_new_impl(STR_DATA(str), STR_SIZE(str));
}

bool strbuf_concat_str(StrBuf *buf, Str str)
{
    return strbuf_concat_impl(buf, STR_DATA(str), STR_SIZE(str));
}

bool strbuf_concat_cstr(StrBuf *buf, const char *cstr)
{
    return strbuf_concat_impl(buf, cstr, StringLength(cstr));
}

bool strbuf_concat_sized(StrBuf *buf, const char *cstr, s32 str_sz)
{
  return strbuf_concat_impl(buf, cstr, str_sz);
}

bool strbuf_concat_impl(StrBuf *buf, const char *cstr, s32 str_sz)
{
  ArenaAllocator *arena;

  if(!buf || !cstr) 
    return false;

  u64 new_concat_size = str_sz;
  if(buf->dynamic) {
    u64 current_size = buf->dyn_buf.size;
    u64 current_cap  = buf->dyn_buf.cap;
    if(current_size  + new_concat_size >= current_cap) {
      current_size += new_concat_size;
      current_cap  =  current_size;
      current_cap  = AlignPow2(current_cap, STRBUF_ALIGNMENT);
      arena        = arenaNew(.commited_size=current_size,.reserved_size=current_cap);
      u8 *ptr      = push_arena_array_zero_aligned(arena, u8, current_size, STRBUF_ALIGNMENT);
      if(!ptr) {
        arenaRelease(arena);
        return false;
      } 
      MemoryCopy(ptr, buf->dyn_buf.ptr, buf->dyn_buf.size);
      MemoryCopy(ptr + buf->dyn_buf.size, cstr, str_sz);
      buf->dyn_buf.ptr  = ptr;
      buf->dyn_buf.size = current_size;
      buf->dyn_buf.cap  = current_cap;
      buf->dyn_buf.last_char = &ptr[current_size];
      arenaRelease(buf->string_arena);
      buf->string_arena = arena;
    } else {
      MemoryCopy(buf->dyn_buf.last_char, cstr, str_sz);
      buf->dyn_buf.size += str_sz;
      buf->dyn_buf.last_char = &buf->dyn_buf.ptr[buf->dyn_buf.size];
    }
  } else {
    if(NEED_TO_ALLOCATE(*buf, str_sz)) {
      u64 fixed_buf_sz = buf->fixed_buf.size;

      char old_copy[EB_CACHE_LINE];
      MemoryCopy(old_copy, buf->fixed_buf.fixed_buf.b, fixed_buf_sz);

      if(!strbuf_alloc(buf, fixed_buf_sz + str_sz, MB(4)))
        return false;

      MemoryCopy(buf->dyn_buf.ptr, old_copy, fixed_buf_sz);
      MemoryCopy(buf->dyn_buf.ptr+fixed_buf_sz, cstr, str_sz);
      buf->dyn_buf.last_char = &buf->dyn_buf.ptr[buf->dyn_buf.size];
    } else {
      MemoryCopy(buf->fixed_buf.last_char, cstr, str_sz);
      buf->fixed_buf.size += str_sz; 
      buf->fixed_buf.last_char = &buf->fixed_buf.fixed_buf.b[buf->fixed_buf.size];
    }
  }

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

    ret = strbuf_concat_sized(buf, used_buf, size);
end:
    scrachEnd(sc);
    return ret;
}

bool strbuf_clear(StrBuf *buf)
{
  if(!buf)
    return false;
  if(buf->dynamic) {
      arenaClear(buf->string_arena); 
      strbuf_alloc(buf, buf->dyn_buf.size, buf->dyn_buf.cap);
      buf->dyn_buf.size = 0;
      buf->dyn_buf.last_char = &buf->dyn_buf.ptr[0];
  } else {
      MemoryZeroStaticArray(buf->fixed_buf.fixed_buf.b);
      buf->fixed_buf.size = 0;
      buf->fixed_buf.last_char = &buf->fixed_buf.fixed_buf.b[0];
  }
  return true; 
}

Str strbuf_get_str(StrBuf *buf)
{
Str ret = {0};
  if(buf) {
      if(buf->dynamic) ret = cstr_sized(buf->dyn_buf.ptr, buf->dyn_buf.size);
      else             ret = cstr_sized(buf->fixed_buf.fixed_buf.b, buf->fixed_buf.size);
  }
  return ret;
}

void strbuf_release(StrBuf *buf)
{
  if(!buf)
    return;

  arenaRelease(buf->string_arena);
  *buf = strbuf_empty();
}
#endif

#if __cplusplus
}
#endif

