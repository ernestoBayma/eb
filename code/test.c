#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

#include "http.h"
#include "allocator.h"
#include "util.h"

#define LOOP_AMOUNT 10

#define EB_STR_BUF_IMPLEMENTATION
#include "str_buf.h"

#define EB_STR_IMPLEMENTATION
#define EB_STR_LIST_IMPLEMENTATION
#include "str.h"

global_var KeyValue usageOptions[] = {
  {"--help", "This usage message"},
  {"--count", "This to run benchmarks"},
  {0,0}
};

void bench_force_cache_miss()
{
#define SIZE 1024
s64 buffer[SIZE];
s32 stride = Bytes(128);
s32 index, index_inner;

  for(index = 0; index < SIZE; index++) {
      buffer[index] = index;
  }
  for(index = 0; index < stride; index++) {
      for(index_inner = index; index_inner < SIZE; index_inner += stride) {
          buffer[index_inner] = buffer[index_inner] % 20;
      }
  }
}

#define BENCH_FUNC(name, bench_code, init_code...) \
s64 bench_##name() { \
s64 macro_var(counter);\
init_code;\
TimedScope({{bench_code}}, macro_var(counter));\
return macro_var(counter);\
}

#define BENCH_FUNC_DEF(name) s64 name(void)
typedef BENCH_FUNC_DEF(bench_func);

typedef struct bench_ctx {
  bench_func *func;
  char       *name;
  s64         times[2];
} BenchCtx;

BENCH_FUNC(memoryCopy_Unaligned, 
{
  memoryCopy(_mem_src, _mem_dst, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World!", _mem_dst[64] = {'A'};
);
BENCH_FUNC(memoryCopy_Aligned, 
{
  memoryCopy(_mem_src, _mem_dst, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World Dude", _mem_dst[64] = {'A'};
);
BENCH_FUNC(memcpy_Unaligned, 
{
  memcpy(_mem_dst, _mem_src, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World!", _mem_dst[64] = {'A'};
);
BENCH_FUNC(memcpy_Aligned, 
{
  memcpy(_mem_dst, _mem_src, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World Dude", _mem_dst[64] = {'A'};
);
BENCH_FUNC(zeroMemory,
{
  zeroMemory(mem, sizeof(mem));
},
  char mem[64];
);
BENCH_FUNC(memset,
{
  memset(mem, 0, sizeof(mem));
},
 char mem[64];
);

BENCH_FUNC(CopyingStrBuf,
{
    MemoryCopy(&a, &b, sizeof(StrBuf));
},
 StrBuf a,b;
);

BENCH_FUNC(ZeroStrBuf,
{
    MemoryZeroStruct(&a);
},
 StrBuf a;
);

global_var 
BenchCtx 
bench_functions[] = {
  {.func=bench_memoryCopy_Unaligned,.name=stringfy(bench_memoryCopy_Unaligned)},
  {.func=bench_memoryCopy_Aligned,.name=stringfy(bench_memoryCopy_Aligned)},
  {.func=bench_memcpy_Unaligned,.name=stringfy(bench_memcpy_Unaligned)},
  {.func=bench_memcpy_Aligned,.name=stringfy(bench_memcpy_Aligned)},
  {.func=bench_zeroMemory,.name=stringfy(bench_zeroMemory)},
  {.func=bench_memset,.name=stringfy(bench_memset)},
  {.func=bench_ZeroStrBuf,.name=stringfy(bench_ZeroStrBuf)},
  {.func=bench_CopyingStrBuf,.name=stringfy(bench_CopyingStrBuf)},
};

void run_benches(BenchCtx *c, s32 total_benches, s32 total_times)
{
s32 index, times, loop_counter;
s64 cycles, min_cycles, max_cycles;
  
  while(loop_counter < LOOP_AMOUNT) {
    for(index = 0; index < total_benches; index++) {
      min_cycles = max_cycles = -1;
      for(times = 0; times < total_times; times++) {
        bench_force_cache_miss();
        cycles = c[index].func();
        if(min_cycles == -1) min_cycles = cycles;
        if(max_cycles == -1) max_cycles = cycles;

        if(cycles < min_cycles) min_cycles = cycles;
        else if(cycles > max_cycles) max_cycles = cycles;
      }
      c[index].times[0] = min_cycles;
      c[index].times[1] = max_cycles; 
    }
    loop_counter++;
  }
  for(index = 0; index < total_benches; index++) {
    fprintf(stderr, "%s\n\tbest perf: %llu cycles\n\tworst perf: %llu cycles\n", c[index].name, c[index].times[0], c[index].times[1]); 
  }
}

int main(int argc, char *argv[])
{
s32 total;

  if(argc > 1) {
      if(MemoryMatch(argv[1], "--count", StringLength(argv[1]))) {
          s32 converted = atoi(argv[2]);
          if(converted > 0) total = converted;
          else {
              fprintf(stderr, "--count expects a value bigger than zero. Leaving....\n");
              exit(0);
          }
      } else {
         printUsage(argv[0], 0, stdout, usageOptions);
      }
  } else {
      printUsage(argv[0], 0, stdout, usageOptions);
  }

  run_benches(bench_functions, StaticArraySize(bench_functions), total);

  return 0;
}
