#if !defined(EB_STR_INCLUDE_H)
#define EB_STR_INCLUDE_H

#include "core.h"

#if defined(Lang_CPP)
C_Linkage_Start
#endif

// Declaration for types
typedef struct Str Str;
struct Str 
{
    const char *readonly_ptr;
    size_t     size;
};
#define STRFMT "%.*s"
#define STR_PRINT_ARGS(str) ((str).size), ((str).readonly_ptr)
#define STR_INVALID ((Str){.readonly_ptr=0,.size=0})
#define STR_DATA(str) (str).readonly_ptr
#define STR_SIZE(str) (str).size
#define STR_IS_EMPTY(s) STR_SIZE(s) <= 0
#define STR_PTR_VALID(s) STR_DATA(s) != 0
#define STR_ALIAS(o) ((Str){.readonly_ptr=(o).readonly_ptr,.size=(o).size})
#define cstr_lit(cstr) ((Str){.readonly_ptr=(cstr),.size=sizeof(cstr)-1})
#define cstr_static(cstr) cstr_lit(cstr)
#define cstr(c) (c)!= 0 ? ((Str){.readonly_ptr=(c),.size=cstr_len((c))}) : STR_INVALID
#define cstr_sized(c,s) (c)!=0? ((Str){.readonly_ptr=(c),.size=(s)}) : STR_INVALID

#ifndef tolower
#define tolower cstr_to_lower
#endif

Str str_substr(Str orig, int start, int end);
Str str_split_index(Str *ptr, size_t index);
bool str_contains(Str s1, Str s2);
int str_pop_first_char(Str *ptr);
Str str_find_first_strarg(Str s, Str to_find);
Str str_find_first_cstrarg(Str s, const char *to_find);
bool str_is_match_strarg(Str s1, Str s2);
bool str_is_match_cstrarg(Str s1, const char *s2);
bool str_is_match_no_case_strarg(Str s1, Str s2);
bool str_is_match_no_case_cstrarg(Str s1, const char *s2);
bool str_starts_with_strarg(Str s1, Str s2);
bool str_starts_with_cstrarg(Str s1, const char *s2);
bool str_starts_with_no_case_strarg(Str s1, Str s2);
bool str_starts_with_no_case_cstrarg(Str s1, const char *s2);
int str_compare(Str s1, Str s2);
Str str_trim_start_strarg(Str s1, Str chars_to_trim);
Str str_trim_start_cstrarg(Str s1, const char* chars_to_trim);
Str str_trim_end_strarg(Str s1, Str chars_to_trim);
Str str_trim_end_cstrarg(Str s1, const char* chars_to_trim);
Str str_trim_strarg(Str s1, Str chars_to_trim);
Str str_trim_cstrarg(Str s1, const char *chars_to_trim);
char *str_to_cstr(char *dst, size_t size, Str s);
Str str_find_last_strarg(Str s, Str to_find);
Str str_find_last_cstrarg(Str s, const char *to_find);
Str str_split_first_delim_strarg(Str *ptr, Str delims);
Str str_split_first_delim_cstrarg(Str *ptr, const char *delims);
int str_split_all_strarg(int dst_size, Str dst[dst_size], Str src, Str delims);
int str_split_all_cstrarg(int dst_size, Str dst[dst_size], Str src, const char* delims);
Str str_split_line(Str *ptr, char *end_of_line);

#define str_find_first(s, to_find) _Generic((to_find),\
  const char *: str_find_first_cstrarg,\
  char *:       str_find_first_cstrarg,\
  Str:          str_find_first_strarg\
)(s,to_find)

#define str_find_last(s, to_find) _Generic((to_find),\
  const char *: str_find_last_cstrarg,\
  char *:       str_find_last_cstrarg,\
  Str:          str_find_last_strarg\
)(s,to_find)

#define str_split_first_delim(s1,s2) _Generic((s2),\
  const char*: str_split_first_delim_cstrarg,\
  char*: str_split_first_delim_cstrarg,\
  Str: str_split_first_delim_strarg\
)(s1,s2)

#define str_split_all(dst_size, dst, src, delims) _Generic((delims),\
  const char *: str_split_all_cstrarg,\
  char *: str_split_all_cstrarg,\
  Str: str_split_all_strarg\
)(dst_size, dst, src, delims)

#define str_is_match(s1,s2) _Generic((s2),\
  const char *: str_is_match_cstrarg,\
  char *: str_is_match_cstrarg,\
  Str: str_is_match_strarg\
)(s1,s2)

#define str_is_match_no_case(s1,s2) _Generic((s2),\
  const char *: str_is_match_no_case_cstrarg,\
  char *: str_is_match_no_case_cstrarg,\
  Str: str_is_match_no_case_strarg\
)(s1,s2)

#define str_starts_with(s1,s2) _Generic((s2),\
  const char *: str_starts_with_cstrarg,\
  char *: str_starts_with_cstrarg,\
  Str: str_starts_with_strarg\
)(s1,s2)

#define str_starts_with_no_case(s1,s2) _Generic((s2), \
  const char *: str_starts_with_no_case_cstrarg,\
  char *: str_starts_with_no_case_cstrarg, \
  Str: str_starts_with_no_case_strarg\
)(s1,s2)

#define str_trim_start(s1,s2) _Generic((s2), \
 const char *: str_trim_start_cstrarg,\
 char *: str_trim_start_cstrarg,\
 Str: str_trim_start_strarg\
)(s1,s2)

#define str_trim_end(s1,s2) _Generic((s2), \
 const char *: str_trim_end_cstrarg,\
 char *: str_trim_end_cstrarg,\
 Str: str_trim_end_strarg\
)(s1,s2)

#define str_trim(s1,s2) _Generic((s2), \
 const char *: str_trim_cstrarg,\
 char *: str_trim_cstrarg,\
 Str: str_trim_strarg\
)(s1,s2)

#ifdef __cplusplus
}
#endif
#endif // EB_STR_INCLUDE_H

#if defined(EB_STR_IMPLEMENTATION)
#if defined(Lang_CPP)
C_Linkage_Start
#endif
priv_func int contains_char(Str s, char c, bool case_sensitive)
{
int result = 0;
  const char *ptr = s.readonly_ptr;
  while(!result && ptr != &s.readonly_ptr[s.size]) {
      if(case_sensitive) 
            result = *ptr == c;
      else 
            result = tolower(*ptr) == tolower(c);
      ptr++;
  }
  return result;
}
priv_func Str split_index(Str *ptr, size_t index)
{
Str   result, remainder;
bool  neg;

    result    = STR_INVALID;
    remainder = *ptr;
    neg       = index < 0;

    if(neg) 
      index = ptr->size + index;

    if(index < 0) 
      index = 0;
    else if(index > ptr->size) 
      index = ptr->size;

    result.readonly_ptr     = remainder.readonly_ptr; result.size = index;
    remainder.readonly_ptr += index; remainder.size -= index;
    
    if(!neg) 
      *ptr    = remainder;
    else {
      *ptr    = result;
      result  = remainder;
    }
    return result;
}
priv_func Str split_first_delim(Str *ptr, Str delims, bool case_sensitive)
{
Str result;
bool found;
const char *c_ptr;

  found = false;
  c_ptr = ptr->readonly_ptr;

  if(ptr->readonly_ptr && STR_PTR_VALID(delims)) {
    while(c_ptr != &ptr->readonly_ptr[ptr->size] && !found) {
      found = contains_char(delims, c_ptr[0], case_sensitive);
      c_ptr += !found;
    }
  }
  if(found) {
    STR_DATA(result) = ptr->readonly_ptr;
    STR_SIZE(result) = c_ptr - ptr->readonly_ptr;
    ptr->readonly_ptr = c_ptr;
    ptr->size -= STR_SIZE(result);

    ptr->size--;
    if(ptr->size) 
      ptr->readonly_ptr++;
  } else {
    result = *ptr;
    *ptr = STR_INVALID;
  }
  return result;
}
priv_func Str split_last_delim(Str *ptr, Str delims, bool case_sensitive)
{
Str result;
bool found;
const char *c_ptr;

  found = false;

  if(ptr->readonly_ptr && ptr->size && STR_PTR_VALID(delims) ) {
    c_ptr = &ptr->readonly_ptr[ptr->size - 1];
    while(c_ptr != ptr->readonly_ptr - 1 && !found) {
      found = contains_char(delims, c_ptr[0], case_sensitive);
      c_ptr -= !found;
    }
  }
  if(found) {
    STR_DATA(result) = ptr->readonly_ptr;
    STR_SIZE(result) = c_ptr - ptr->readonly_ptr;
    ptr->readonly_ptr = c_ptr;
    ptr->size -= STR_SIZE(result);

    ptr->size--;
    if(ptr->size) 
      ptr->readonly_ptr++;
  } else {
    result = *ptr;
    *ptr = STR_INVALID;
  }
  return result;
}
char *str_to_cstr(char *dst, size_t dst_size, Str s)
{
size_t copy_sz, src_sz;
  src_sz = STR_SIZE(s) < 0 ? 0 : STR_SIZE(s);
  if(dst_size && (dst != 0)) {
    if(STR_SIZE(s) < 0) 
      STR_SIZE(s) = 0;
    copy_sz = (dst_size - 1) < src_sz ? (dst_size-1):src_sz;
    memcpy(dst, STR_DATA(s), copy_sz);
    dst[copy_sz] = 0;
  }
  return dst;
}
Str str_split_index(Str *ptr, size_t index)
{
  Str result = STR_INVALID;
  if(ptr)
    result = split_index(ptr, index);
  return result;
}
int str_pop_first_char(Str *ptr)
{
  int result = 0;
  if(ptr && ptr->size > 0)
    result = split_index(ptr, 1).readonly_ptr[0];
  return result;
}
Str str_find_first_strarg(Str s, Str to_find)
{
  Str result = STR_INVALID;
  const char *remaining = STR_DATA(s);
  bool found = false;

  if(STR_PTR_VALID(s) && STR_PTR_VALID(to_find)) {
    while((&STR_DATA(s)[STR_SIZE(s)] - remaining >= STR_SIZE(to_find)) && !found) {
      found = !memcmp(remaining, STR_DATA(to_find), STR_SIZE(to_find));
      remaining += !found; 
    }
  }
  if(found) {
    STR_DATA(result)  = remaining;
    STR_SIZE(result)  = STR_SIZE(to_find);
  }
  return result;
}
Str str_find_first_cstrarg(Str s, const char *to_find)
{
  return str_find_first_strarg(s, cstr(to_find));
}
Str str_find_last_strarg(Str s, Str to_find)
{
  Str result = STR_INVALID;
  const char *remaining = &STR_DATA(s)[STR_SIZE(s)];
  bool found = false;

  if(STR_PTR_VALID(s) && STR_PTR_VALID(to_find)) {
    while((remaining >= STR_DATA(s)) && !found)  {
      found = !memcmp(remaining, STR_DATA(to_find), STR_SIZE(to_find));
      remaining -= !found; 
    }
  }
  if(found) {
    STR_DATA(result)  = remaining;
    STR_SIZE(result)  = STR_SIZE(to_find);
  }
  return result;
}
Str str_find_last_cstrarg(Str s, const char *to_find)
{
  return str_find_last_strarg(s, cstr(to_find));
}

Str str_split_first_delim_strarg(Str *ptr, Str delims)
{
Str result;
  result = STR_INVALID;
  if(ptr) 
    result = split_first_delim(ptr, delims, 1);
  return result;
}
Str str_split_first_delim_cstrarg(Str *ptr, const char *delims)
{
  return str_split_first_delim_strarg(ptr, cstr(delims));
}
bool str_is_match_strarg(Str s1, Str s2)
{
  return (STR_SIZE(s1) == STR_SIZE(s2)) && (STR_DATA(s1) == STR_DATA(s2) ||
  !memcmp(STR_DATA(s1), STR_DATA(s2), STR_SIZE(s1)));
}
bool str_is_match_cstrarg(Str s1, const char *s2)
{
  return str_is_match_strarg(s1, cstr(s2));
}
bool str_is_match_no_case_strarg(Str s1, Str s2)
{
  return (STR_SIZE(s1) == STR_SIZE(s2)) && (STR_DATA(s1) == STR_DATA(s2) ||
!memcmp_nocase(STR_DATA(s1), STR_DATA(s2), STR_SIZE(s1)));
}
bool str_is_match_no_case_cstrarg(Str s1, const char *s2)
{
  return str_is_match_no_case_strarg(s1, cstr(s2));
}
bool str_starts_with_strarg(Str s1, Str s2)
{
bool result;

  if(!STR_PTR_VALID(s2))
    result = !STR_PTR_VALID(s1);
  else 
    result = (STR_SIZE(s1) >= STR_SIZE(s2)) && (STR_DATA(s1) == STR_DATA(s2) ||
!memcmp(STR_DATA(s1), STR_DATA(s2), STR_SIZE(s2)) );
  return result;
}
bool str_starts_with_cstrarg(Str s1, const char *s2)
{
  return str_starts_with_strarg(s1, cstr(s2));
}
bool str_starts_with_no_case_strarg(Str s1, Str s2)
{
bool result;

  if(!STR_PTR_VALID(s2))
    result = !STR_PTR_VALID(s1);
  else 
    result = (STR_SIZE(s1) >= STR_SIZE(s2)) && (STR_DATA(s1) == STR_DATA(s2) ||
!memcmp_nocase(STR_DATA(s1), STR_DATA(s2), STR_SIZE(s2)) );
  return result;
}
bool str_starts_with_no_case_cstrarg(Str s1, const char *s2)
{
  return str_starts_with_no_case_strarg(s1, cstr(s2));
}
bool str_contains(Str s1, Str s2)
{
  return STR_PTR_VALID(str_find_first(s1, s2));
}
int str_compare(Str s1, Str s2)
{
int used_size, result;
  used_size = STR_SIZE(s1) < STR_SIZE(s2) ? STR_SIZE(s1) : STR_SIZE(s2);
  result = 0;
    if(used_size) 
      result = memcmp(STR_DATA(s1), STR_DATA(s2), used_size);
  if(!result && STR_SIZE(s1) != STR_SIZE(s2))
      result = STR_SIZE(s1) > STR_SIZE(s2) ? 1 : -1;
  return result;
}
Str str_trim_start_strarg(Str s1, Str chars_to_trim)
{
  while(STR_SIZE(s1) && contains_char(chars_to_trim, STR_DATA(s1)[0], 1)) {
    STR_DATA(s1)++;
    STR_SIZE(s1)--;
  }
  return s1;
}
Str str_trim_start_cstrarg(Str s1, const char* chars_to_trim)
{
  return str_trim_start_strarg(s1, cstr(chars_to_trim));
}
Str str_trim_end_strarg(Str s1, Str chars_to_trim)
{
  while(STR_SIZE(s1) && 
      contains_char(chars_to_trim, STR_DATA(s1)[STR_SIZE(s1)-1], 1)) {
    STR_SIZE(s1)--;
  }
  return s1;
}
Str str_trim_end_cstrarg(Str s1, const char* chars_to_trim)
{
  return str_trim_end_strarg(s1, cstr(chars_to_trim));
}
Str str_trim_strarg(Str s1, Str chars_to_trim)
{
  s1 = str_trim_start(s1, chars_to_trim);
  s1 = str_trim_end(s1, chars_to_trim);
  return s1;
}
Str str_trim_cstrarg(Str s1, const char *chars_to_trim)
{
  return str_trim_strarg(s1, cstr(chars_to_trim));
}
int str_split_all_strarg(int dst_size, Str dst[dst_size], Str src, Str delims)
{
int count = 0;
  while(STR_PTR_VALID(src) && count < dst_size)
    dst[count++] = str_split_first_delim_strarg(&src, delims);
  return count;
}
int str_split_all_cstrarg(int dst_size, Str dst[dst_size], Str src, const char* delims)
{
  return str_split_all_strarg(dst_size, dst, src, cstr(delims));
}
Str str_split_line(Str *ptr, char *end_of_line)
{
Str result = STR_INVALID, src;
char e = 0;
  
  if(ptr && ptr->size) {
    src = *ptr;
    if(end_of_line && end_of_line[0]) {
        if(end_of_line[0] + STR_DATA(src)[0] == '\r'+'\n')
          str_pop_first_char(&src);
    }
    result = str_split_first_delim(&src, cstr("\r\n"));
    if(STR_PTR_VALID(src)) {
      e = STR_DATA(result)[STR_SIZE(result)];
      if(e + STR_DATA(src)[0] == '\r'+'\n') {
        str_pop_first_char(&src);
        e = 0;
      }
      if(end_of_line)
        *end_of_line = e;

      *ptr = src;
    } else {
      *ptr = result;
      result = STR_INVALID;
    }
  }
  return result;
}
Str str_substr(Str orig, int start, int end)
{
    Str result = STR_ALIAS(orig);
    if(!STR_IS_EMPTY(orig) && STR_PTR_VALID(orig)) {
        if(start < 0) start = STR_SIZE(orig) + start;
        if(end   < 0) end   = STR_SIZE(orig) + end;

        if(start <= end && start < orig.size && end >= 0) {
            start = Max(start, 0); 
            end   = Min(end, STR_SIZE(orig));

            STR_DATA(result) = &STR_DATA(orig)[start];
            STR_SIZE(result) = end - start;
        }
    }
    return result;
}

#if defined(Lang_CPP)
C_Linkage_End
#endif
#endif

