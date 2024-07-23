#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "core.h"
#include "str.h"
#include "os.h"
#include "util.h"

int cstr_to_lower(int c)
{
  return c >= 'A' && c <= 'Z' ? c + 32 : c;
}

priv_func
bool is_aligned(void *ptr, s32 alignment)
{
  return ((uintptr_t)ptr % alignment) == 0;
}

priv_func
bool is_aligned_16_bytes(void *ptr)
{
  return is_aligned(ptr, 16);
}

priv_func
void TARGET_SSSE3 zeroMemorySSE(void *ptr, s64 amt)
{
  if(!is_aligned(ptr, EB_SSE_BOUNDARY_BYTES))  return; 
  m128i zero = _mm_setzero_si128(), ptr_as_sse_type;
  u8 *ptr_as_bytes = Cast(u8*, ptr);

  amt /= EB_SSE_BOUNDARY_BYTES;
  while(amt--) {
    ptr_as_sse_type = _mm_load_si128((m128i*)(&ptr_as_bytes[0])); 
    _mm_store_si128(&ptr_as_sse_type, zero);
    ptr_as_bytes += EB_SSE_BOUNDARY_BYTES;
  }
}

void zeroMemory(void *ptr, s64 amt)
{
  if(!ptr || amt <= 0) return; 
#if defined(EB_USE_MEMSET)
    memset(ptr, 0, amt);
    return;
#elif defined(EB_HAS_SSE)
    zeroMemorySSE(ptr, amt); 
#endif

  u8 *c = Cast(u8*, ptr);
  while(amt--) {
   *c++ = 0;
  }
}

priv_func
void TARGET_SSSE3 memoryCopySSE(void **src, void **dest, s64 size)
{
u8    *src_as_bytes, *dest_as_bytes;
u8    *src_top, *dest_top;

  src_top  = Cast(u8*, *src);
  dest_top = Cast(u8*, *dest);

  src_as_bytes  = src_top;
  dest_as_bytes = dest_top;
  if(is_aligned_16_bytes(src_top) && is_aligned_16_bytes(dest_top)) 
  {
    m128i  *src_as_sse_type, *dest_as_sse_type; 

    size /= EB_SSE_BOUNDARY_BYTES;
    while(size-- > 0) {
      src_as_sse_type  = ((m128i*)src_as_bytes);
      dest_as_sse_type = ((m128i*)dest_as_bytes);

      _mm_store_si128(dest_as_sse_type++, _mm_load_si128(src_as_sse_type++));
      src_as_bytes  += EB_SSE_BOUNDARY_BYTES;
      dest_as_bytes += EB_SSE_BOUNDARY_BYTES;
    }
  }
  while(size-- > 0)
   *dest_as_bytes++ = *src_as_bytes++;

   *src   = Cast(void*, src_top);
   *dest  = Cast(void*, dest_top);
}

priv_func
INLINE
void memoryCopyUnrolled(void *src, void *dest, s64 size)
{
u8  *src_as_bytes, *dest_as_bytes;
u8  src_b_1, src_b_2, src_b_3, src_b_4;

  src_as_bytes  = Cast(u8*, src);
  dest_as_bytes = Cast(u8*, dest);
  size /= 4;
  while(size--) {
    src_b_1 = src_as_bytes[0];
    src_b_2 = src_as_bytes[1];
    src_b_3 = src_as_bytes[2];
    src_b_4 = src_as_bytes[3];
    
    dest_as_bytes[0] = src_b_1;
    dest_as_bytes[1] = src_b_2;
    dest_as_bytes[2] = src_b_3;
    dest_as_bytes[3] = src_b_4;

    src_as_bytes += 4;
    dest_as_bytes += 4;
  }
  while(size--)
    *dest_as_bytes++ = *src_as_bytes++;

}


void memoryCopy(void *pa, void *pb, s64 amt)
{
  if(!pa || !pb) return;
  if( amt <= 0 ) return;

#if defined(EB_USE_MEMCPY)
  memcpy(pb, pa, amt);
#elif defined(EB_HAS_SSE) 
  memoryCopySSE(&pa, &pb, amt);
#elif defined(EB_UNROLLED_MEMCPY)
  memoryCopyUnrolled(pa, pb, amt);
#endif
}

int memcmp_nocase(char *str1, char *str2, size_t amt)
{
  int result = 0;
  while(amt-- && !result) 
      result += cstr_to_lower((unsigned)(*str1++)) - cstr_to_lower((unsigned)(*str2++));
  return result;
}

int cstr_len(char *ptr)
{
  int result = 0;
  if(ptr) {
    while(*ptr++) 
      result++;
  }
  return result;
}

bool humanRedableFileRequestError(Str str, enum FileRequestErrors error)
{
  char *st_str = "";
  switch(error) {
  case FileRequestSuccess:
    st_str = "Success";
  break;
  case FileRequestNotRegularFile:
    st_str = "Not a normal file";
  break;
  case FileRequestNotFound:
    st_str = "File not found";
  break;
  case FileRequestNoPermission:
    st_str = "No right permissions on the file for this user";
  break; 
  case FileRequestUnkownError:
    st_str = "Unknown error";
  break;
  case FileRequestNoReadPermission:
    st_str = "Read permission was not requested for this file";
  break;
  case FileRequestNoWritePermission:
    st_str = "Write permission was not requested for this file";
  break;
  case FileRequestIsADirectory:
    st_str = "This file is a directory when it should not be";
  break; 
  }
  strncat(STR_DATA(str), st_str, STR_SIZE(str));
  return true;
}

void printUsage(char *prog_name, char *version, FILE *out, KeyValue *values)
{
  fprintf(out, "Usage: %s [OPTIONS] ...\n", prog_name);
  if(version)
    fprintf(out, "Version: %s\n", version);

  if(values) {
    fprintf(out, "Options:\n");
    for(int i = 0; values[i].key; i++) {
      UsagePrint(out, values[i].key, values[i].value);
    }
  }
  exit(EXIT_SUCCESS);
}
