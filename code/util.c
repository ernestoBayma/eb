#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "core.h"
#include "os.h"
#include "str.h"
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
void TARGET_SSE zeroMemorySSE(void *ptr, s64 amt)
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
void TARGET_SSE memoryCopySSE(void **src, void **dest, s64 size)
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

u64
util_node_hash(void *data, u64 data_size)
{
u8  *bytes;
u64 result = 5381;
    bytes = Cast(u8*, data);
    for(u64 i = 0; i < data_size; i+= 1) {
      result = ((result << 5) + result + bytes[i]);
    }
    return result;
}

#if defined(EB_HAS_AVX) 

#if defined(COMPILER_GCC)
  #if defined(_RELEASE_BUILD)
    #pragma GCC optimize("Ofast,unroll-loops")
  #endif
  #pragma GCC target("tune=native")
#endif

#include <tmmintrin.h>
#include <emmintrin.h> 
#include <wmmintrin.h>  //for intrinsics for AES-NI

// Based on MeowHash
#define PAGESIZE 4096
#define PREFETCH 4096
#define PREFETCH_LIMIT 0x3ff

static u8 shift_adjust[32] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static u8 mask_len[32] = {255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
// PI encoded
static u8 hash_seed[128] = 
{
0x32, 0x43, 0xF6, 0xA8, 0x88, 0x5A, 0x30, 0x8D,
0x31, 0x31, 0x98, 0xA2, 0xE0, 0x37, 0x07, 0x34,
0x4A, 0x40, 0x93, 0x82, 0x22, 0x99, 0xF3, 0x1D,
0x00, 0x82, 0xEF, 0xA9, 0x8E, 0xC4, 0xE6, 0xC8,
0x94, 0x52, 0x82, 0x1E, 0x63, 0x8D, 0x01, 0x37,
0x7B, 0xE5, 0x46, 0x6C, 0xF3, 0x4E, 0x90, 0xC6,
0xCC, 0x0A, 0xC2, 0x9B, 0x7C, 0x97, 0xC5, 0x0D,
0xD3, 0xF8, 0x4D, 0x5B, 0x5B, 0x54, 0x70, 0x91,
0x79, 0x21, 0x6D, 0x5D, 0x98, 0x97, 0x9F, 0xB1,
0xBD, 0x13, 0x10, 0xBA, 0x69, 0x8D, 0xFB, 0x5A,
0xC2, 0xFF, 0xD7, 0x2D, 0xBD, 0x01, 0xAD, 0xFB,
0x7B, 0x8E, 0x1A, 0xFE, 0xD6, 0xA2, 0x67, 0xE9,
0x6B, 0xA7, 0xC9, 0x04, 0x5F, 0x12, 0xC7, 0xF9,
0x92, 0x4A, 0x19, 0x94, 0x7B, 0x39, 0x16, 0xCF,
0x70, 0x80, 0x1F, 0x2E, 0x28, 0x58, 0xEF, 0xC1,
0x66, 0x36, 0x92, 0x0D, 0x87, 0x15, 0x74, 0xE6
};

// Move Unaligned Double Quadword
#define prefetch0(A) _mm_prefetch((char*)(A), _MM_HINT_T0)
#define movdqu(A,B) A = _mm_loadu_si128((__m128i *)(B))
#define movq(A, B) A = _mm_set_epi64x(0, B);
#define movdqu_mem(A, B)  _mm_storeu_si128((__m128i *)(A), B)
#define aesdec(A, B)  A = _mm_aesdec_si128(A, B)
#define pshufb(A, B)  A = _mm_shuffle_epi8(A, B)
#define pxor(A, B)    A = _mm_xor_si128(A, B)
#define paddq(A, B) A = _mm_add_epi64(A, B)
#define pand(A, B)    A = _mm_and_si128(A, B)
#define palignr(A, B, i) A = _mm_alignr_epi8(A, B, i)
#define pxor_clear(A, B) A = _mm_setzero_si128();

#define MIX_REG(r1,r2,r3,r4,r5, i1, i2, i3, i4) \
aesdec(r1, r2); \
paddq(r3, i1); \
pxor(r2, i2); \
aesdec(r2, r4); \
paddq(r5, i3); \
pxor(r4, i4);

#define MIX(r1, r2, r3, r4, r5, ptr) \
MIX_REG(r1, r2, r3, r4, r5, _mm_loadu_si128((__m128i*)((ptr + 15))), _mm_loadu_si128((__m128i*)(ptr+0)), _mm_loadu_si128((__m128i*)(ptr + 1)), _mm_loadu_si128((__m128i*)((ptr + 16))))

#define SHUFFLE(r1, r2, r3, r4, r5, r6) \
aesdec(r1,r4); \
paddq(r2, r5); \
pxor(r4, r6); \
aesdec(r4, r2);\
paddq(r5, r6);\
pxor(r2,r3)

#define MIX_BLOCK(xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,rax)\
MIX(xmm0,xmm4,xmm6,xmm1,xmm2, rax + 0x00); \
MIX(xmm1,xmm5,xmm7,xmm2,xmm3, rax + 0x20); \
MIX(xmm2,xmm6,xmm0,xmm3,xmm4, rax + 0x40); \
MIX(xmm3,xmm7,xmm1,xmm4,xmm5, rax + 0x60); \
MIX(xmm4,xmm0,xmm2,xmm5,xmm6, rax + 0x80); \
MIX(xmm5,xmm1,xmm3,xmm6,xmm7, rax + 0xa0); \
MIX(xmm6,xmm2,xmm4,xmm7,xmm0, rax + 0xc0); \
MIX(xmm7,xmm3,xmm5,xmm0,xmm1, rax + 0xe0); \
rax += 0x100;

TARGET_AVX2 TARGET_AES u64 util_hash_sse(void *data, u64 data_size)
{
__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;       // Accumulators
__m128i xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15; // Appended values (residual, length)

  u8 *rax = Cast(u8*, data);
  u8 *rcx = Cast(u8*, hash_seed);

  movdqu(xmm0, rcx + 0x00);
  movdqu(xmm1, rcx + 0x10);
  movdqu(xmm2, rcx + 0x20);
  movdqu(xmm3, rcx + 0x30);

  movdqu(xmm4, rcx + 0x40);
  movdqu(xmm5, rcx + 0x50);
  movdqu(xmm6, rcx + 0x60);
  movdqu(xmm7, rcx + 0x70);

  long int block_count = (data_size >> 8);
  if(block_count > PREFETCH_LIMIT) {
    while(block_count--) {
        prefetch0(rax + PREFETCH + 0x00);
        prefetch0(rax + PREFETCH + 0x40);
        prefetch0(rax + PREFETCH + 0x80);
        prefetch0(rax + PREFETCH + 0xc0);

        MIX_BLOCK(xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7, rax);
    }
  } else {
    while(block_count--) {
#if 1
        MIX_BLOCK(xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7, rax);
#else
        MIX(xmm0,xmm4,xmm6,xmm1,xmm2, rax + 0x00);
        MIX(xmm1,xmm5,xmm7,xmm2,xmm3, rax + 0x20);
        MIX(xmm2,xmm6,xmm0,xmm3,xmm4, rax + 0x40);
        MIX(xmm3,xmm7,xmm1,xmm4,xmm5, rax + 0x60);
        MIX(xmm4,xmm0,xmm2,xmm5,xmm6, rax + 0x80);
        MIX(xmm5,xmm1,xmm3,xmm6,xmm7, rax + 0xa0);
        MIX(xmm6,xmm2,xmm4,xmm7,xmm0, rax + 0xc0);
        MIX(xmm7,xmm3,xmm5,xmm0,xmm1, rax + 0xe0);

        rax += 0x100;
#endif
    }
  }

  pxor_clear(xmm9, xmm9);
  pxor_clear(xmm11, xmm11);

  // Load the part that is not 16-byte aligned
  u8 *last = Cast(u8*, data) + (data_size & ~0xf);
  u32 len_8 = (data_size & 0xf);
  if(len_8) 
  {
    movdqu(xmm8, &mask_len[0x10 - len_8]);

    u8 *last_ok = Cast(u8*, (((Cast(u64, (Cast(u8*,data) + data_size - 1)) | (PAGESIZE - 1)) - 16)));
    int align = (last > last_ok) ? ((int)(s64)last) & 0xf : 0;

    movdqu(xmm10, &shift_adjust[align]);
    movdqu(xmm9, last - align);
    pshufb(xmm9, xmm10);

    // and off the extra bytes
    pand(xmm9, xmm8);
  }

  if(data_size & 0x10) {
    xmm11 = xmm9;
    movdqu(xmm9, last - 0x10);
  }

  xmm8 = xmm9;
  xmm10 = xmm9;
  palignr(xmm8, xmm11, 15);
  palignr(xmm10, xmm11, 1);

  pxor_clear(xmm12, xmm12);
  pxor_clear(xmm13, xmm13);
  pxor_clear(xmm14, xmm14);

  movq(xmm15, data_size);

  palignr(xmm12, xmm15, 15);
  palignr(xmm14, xmm15, 1);

  MIX_REG(xmm0, xmm4, xmm6, xmm1, xmm2, xmm8, xmm9, xmm10, xmm11);
  MIX_REG(xmm1, xmm5, xmm7, xmm2, xmm3, xmm12, xmm13, xmm14, xmm15);

  u32 lane_count = (data_size >> 5) & 0x7;
  if(lane_count == 0) goto mix_down; MIX(xmm2,xmm6,xmm0,xmm3,xmm4, rax + 0x00); --lane_count;
  if(lane_count == 0) goto mix_down; MIX(xmm3,xmm7,xmm1,xmm4,xmm5, rax + 0x20); --lane_count;
  if(lane_count == 0) goto mix_down; MIX(xmm4,xmm0,xmm2,xmm5,xmm6, rax + 0x40); --lane_count;
  if(lane_count == 0) goto mix_down; MIX(xmm5,xmm1,xmm3,xmm6,xmm7, rax + 0x60); --lane_count;
  if(lane_count == 0) goto mix_down; MIX(xmm6,xmm2,xmm4,xmm7,xmm0, rax + 0x80); --lane_count;
  if(lane_count == 0) goto mix_down; MIX(xmm7,xmm3,xmm5,xmm0,xmm1, rax + 0xa0); --lane_count;
  if(lane_count == 0) goto mix_down; MIX(xmm0,xmm4,xmm6,xmm1,xmm2, rax + 0xc0); --lane_count;
  
  mix_down:

  SHUFFLE(xmm0, xmm1, xmm2, xmm4, xmm5, xmm6);
  SHUFFLE(xmm1, xmm2, xmm3, xmm5, xmm6, xmm7);
  SHUFFLE(xmm2, xmm3, xmm4, xmm6, xmm7, xmm0);
  SHUFFLE(xmm3, xmm4, xmm5, xmm7, xmm0, xmm1);
  SHUFFLE(xmm4, xmm5, xmm6, xmm0, xmm1, xmm2);
  SHUFFLE(xmm5, xmm6, xmm7, xmm1, xmm2, xmm3);
  SHUFFLE(xmm6, xmm7, xmm0, xmm2, xmm3, xmm4);
  SHUFFLE(xmm7, xmm0, xmm1, xmm3, xmm4, xmm5);
  SHUFFLE(xmm0, xmm1, xmm2, xmm4, xmm5, xmm6);
  SHUFFLE(xmm1, xmm2, xmm3, xmm5, xmm6, xmm7);
  SHUFFLE(xmm2, xmm3, xmm4, xmm6, xmm7, xmm0);
  SHUFFLE(xmm3, xmm4, xmm5, xmm7, xmm0, xmm1);

  paddq(xmm0, xmm2);
  paddq(xmm1, xmm3);
  paddq(xmm4, xmm6);
  paddq(xmm5, xmm7);
  pxor(xmm0, xmm1);
  pxor(xmm4, xmm5);
  paddq(xmm0, xmm4);

  return (u64)_mm_extract_epi64(xmm0, 0);
}

#undef prefetch0
#undef movdqu
#undef movq
#undef movdqu_mem
#undef aesdec
#undef pshufb
#undef pxor
#undef paddq
#undef pand
#undef palignr
#undef pxor_clear
#undef MIX_REG
#undef MIX
#undef SHUFFLE

#else
u64
util_hash_sse(void *data, u64 data_size)
{
  return util_node_hash(data, data_size);
}
#endif
