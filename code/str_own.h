#if !defined(EB_STR_OWN_H)
#define EB_STR_OWN_H

#include "core.h"
#include "str.h"
#include "allocator.h"

#if defined(Lang_CPP)
C_Linkage_Start
#endif

typedef struct StrOwn StrOwn;
struct StrOwn 
{
    Str str;
};

StrOwn str_own_copy_cstrarg(ArenaAllocator *arena, const char *c_str);
StrOwn str_own_copy_strarg(ArenaAllocator *arena, Str str);

#define str_own_copy(arena, s) _Generic((s),\
 const char*: str_own_copy_cstrarg,\
 char*: str_own_copy_cstrarg,\
 Str: str_own_copy_strarg\
)(arena,s)

#define STR_OWN_DATA(str_own) (str_own).str
#define STR_OWN_SIZE(str_own) STR_SIZE((str_own).str)
#define STR_OWN_GET_PTR(str_own) (str_own).ptr

#if defined(Lang_CPP)
C_Linkage_End
#endif
#endif // EB_STR_OWN_H


#if defined(EB_STR_OWN_IMPLEMENTATION)
#if defined(Lang_CPP)
C_Linkage_Start
#endif

StrOwn str_own_copy_strarg(ArenaAllocator *arena, Str str)
{
StrOwn result;
char *ptr;

  MemoryZeroStruct(&result);
  if(STR_PTR_VALID(str) && STR_SIZE(str) > 0) {
    size_t size = STR_SIZE(str);
    ptr = (char*)push_arena_array(arena, char, size + 1);
    MemoryCopy(ptr, STR_DATA(str), size);
    ptr[size] = 0;
    result.str = cstr_sized(ptr, size);
  }
  return result;
}
StrOwn str_own_copy_cstrarg(ArenaAllocator *arena, const char *c_str)
{
  return str_own_copy_strarg(arena, cstr(c_str));    
}
#if defined(Lang_CPP)
C_Linkage_End
#endif
#endif // Implementation

