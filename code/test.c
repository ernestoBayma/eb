#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

#include "http.h"
#include "allocator.h"
#include "util.h"
#include "os.h"
#include "timer.h"

#define LOOP_AMOUNT 10

#define EB_STR_BUF_IMPLEMENTATION
#include "str_buf.h"

#define EB_STR_IMPLEMENTATION
#define EB_STR_LIST_IMPLEMENTATION
#include "str.h"

#define EB_LISTS_IMPLEMENTATION
#include "lists.h"

global_var KeyValue usageOptions[] = {
  {"--help", "This usage message"},
  {"--count", "This to run benchmarks"},
  {"--tests", "Run all tests"},
  {0,0}
};

void bench_force_cache_miss()
{
#define SIZE KB(8)
u8 buffer[SIZE];
s32 stride = Bytes(128);
s32 index, index_inner;

  srand(time(NULL));
  for(index = 0; index < SIZE; index++) {
      buffer[index] = index;
  }
  for(index = 0; index < stride; index++) {
      for(index_inner = index; index_inner < SIZE; index_inner += stride) {
          buffer[index_inner] = buffer[rand() % SIZE];
      }
  }
}

typedef struct bench_res {
  s64 cycle;
  TimeValue clock_time;
} BenchResult;

#define BENCH_FUNC_NAME(used_name) bench_##used_name
#define BENCH_FUNC_NAME_STR(func_name) stringfy(BENCH_FUNC_NAME(func_name))

#define BENCH_FUNC_CALLBACK(name) BenchResult name(void)
typedef BENCH_FUNC_CALLBACK(bench_func);

#define BENCH_FUNC(name, bench_code, init_code...)\
BenchResult bench_##name(){\
BenchResult res = {0};\
  res.clock_time = timerNow();\
  init_code;\
  TimedScope({bench_code;}, res.cycle);\
  res.clock_time = timerDiff(res.clock_time, timerNow());\
  return res;\
}
#define BENCH_FUNC_CACHE_MISS(name, bench_code, init_code...)\
BenchResult bench_##name(){\
BenchResult res = {0};\
  res.clock_time = timerNow();\
  init_code;\
  bench_force_cache_miss();\
  TimedScope({bench_code;}, res.cycle);\
  res.clock_time = timerDiff(res.clock_time, timerNow());\
  return res;\
}

typedef struct bench_ctx {
  bench_func *func;
  char       *name;
  s64         times[2];
  s64         sum_times;
  s64         total_times;
  TimeValue   total_seconds; 
} BenchCtx;

BENCH_FUNC_CACHE_MISS(memoryCopy_Unaligned,{
  memoryCopy(_mem_src, _mem_dst, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World!", _mem_dst[64] = {'A'};
);
BENCH_FUNC_CACHE_MISS(memoryCopy_Aligned,{
  memoryCopy(_mem_src, _mem_dst, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World Dude", _mem_dst[64] = {'A'};
);
BENCH_FUNC_CACHE_MISS(memcpy_Unaligned,{
  memcpy(_mem_dst, _mem_src, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World!", _mem_dst[64] = {'A'};
);
BENCH_FUNC_CACHE_MISS(memcpy_Aligned,{
  memcpy(_mem_dst, _mem_src, strlen(_mem_src) + 1);
},
 char _mem_src[] = "Hello World Dude", _mem_dst[64] = {'A'};
);
BENCH_FUNC_CACHE_MISS(zeroMemory,{
  zeroMemory(mem, sizeof(mem));
},
  char mem[64];
);
BENCH_FUNC_CACHE_MISS(memset,{
  memset(mem, 0, sizeof(mem));
},
 char mem[64];
);

BENCH_FUNC_CACHE_MISS(CopyingStrBuf,{
    MemoryCopy(&a, &b, sizeof(StrBuf));
},
 StrBuf a,b;
);

BENCH_FUNC_CACHE_MISS(ZeroStrBuf,{
    MemoryZeroStruct(&a);
},
 StrBuf a;
);

BENCH_FUNC_CACHE_MISS(cache_miss_sse_hash, 
{
    int array_size = StaticArraySize(max_string);
    for(int i = 0; i < LOOP_AMOUNT; i++) {
        for(int j = 0; j < array_size; j++) {
              int index = rand() % StaticArraySize(all_chars);
              MemorySet(&max_string[j], all_chars[index], sizeof(char));
        }
        util_hash_sse(max_string, array_size);
    }
},
    char all_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXZabcdefghijklmnopqrstuvxz"; 
    char max_string[1024];
    srand(time(NULL));
);

BENCH_FUNC_CACHE_MISS(cache_miss_simple_hash,
{
    int array_size = StaticArraySize(max_string);
    for(int i = 0; i < LOOP_AMOUNT; i++) {
        for(int j = 0; j < array_size; j++) {
              int index = rand() % StaticArraySize(all_chars);
              MemorySet(&max_string[j], all_chars[index], sizeof(char));
        }
        util_node_hash(max_string, array_size);
    }
},
    char all_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXZabcdefghijklmnopqrstuvxz";
    char max_string[1024];
    srand(time(NULL));
);

BENCH_FUNC(sse_hash, 
{
    int array_size = StaticArraySize(max_string);
    for(int i = 0; i < LOOP_AMOUNT; i++) {
        for(int j = 0; j < array_size; j++) {
              int index = rand() % StaticArraySize(all_chars);
              MemorySet(&max_string[j], all_chars[index], sizeof(char));
        }
        util_hash_sse(max_string, array_size);
    }
},
    char all_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXZabcdefghijklmnopqrstuvxz"; 
    char max_string[1024];
    srand(time(NULL));
);

BENCH_FUNC(simple_hash,
{
    int array_size = StaticArraySize(max_string);
    for(int i = 0; i < LOOP_AMOUNT; i++) {
        for(int j = 0; j < array_size; j++) {
              int index = rand() % StaticArraySize(all_chars);
              MemorySet(&max_string[j], all_chars[index], sizeof(char));
        }
        util_node_hash(max_string, array_size);
    }
},
    char all_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVXZabcdefghijklmnopqrstuvxz";
    char max_string[1024];
    srand(time(NULL));
);

global_var 
BenchCtx 
bench_functions[] = {
  {.func=BENCH_FUNC_NAME(memoryCopy_Unaligned),.name=BENCH_FUNC_NAME_STR(memoryCopy_Unaligned)},
  {.func=BENCH_FUNC_NAME(memoryCopy_Aligned),.name=BENCH_FUNC_NAME_STR(memoryCopy_Aligned)},
  {.func=BENCH_FUNC_NAME(memcpy_Unaligned),.name=BENCH_FUNC_NAME_STR(memcpy_Unaligned)},
  {.func=BENCH_FUNC_NAME(memcpy_Aligned),.name=BENCH_FUNC_NAME_STR(memcpy_Aligned)},
  {.func=BENCH_FUNC_NAME(zeroMemory),.name=BENCH_FUNC_NAME_STR(zeroMemory)},
  {.func=BENCH_FUNC_NAME(memset),.name=BENCH_FUNC_NAME_STR(memset)},
  {.func=BENCH_FUNC_NAME(ZeroStrBuf),.name=BENCH_FUNC_NAME_STR(ZeroStrBuf)},
  {.func=BENCH_FUNC_NAME(CopyingStrBuf),.name=BENCH_FUNC_NAME_STR(CopyingStrBuf)},
  {.func=BENCH_FUNC_NAME(sse_hash),.name=BENCH_FUNC_NAME_STR(sse_hash)},
  {.func=BENCH_FUNC_NAME(simple_hash),.name=BENCH_FUNC_NAME_STR(simple_hash)},
  {.func=BENCH_FUNC_NAME(cache_miss_sse_hash),.name=BENCH_FUNC_NAME_STR(cache_miss_sse_hash)},
  {.func=BENCH_FUNC_NAME(cache_miss_simple_hash),.name=BENCH_FUNC_NAME_STR(cache_miss_simple_hash)},
};

static inline void print_bench(BenchCtx *c)
{
u8 backing_buffer[24] = {0};

  timerAsStr(c->total_seconds, backing_buffer, sizeof(backing_buffer));
  fprintf(stderr, "%s\n"
                  "\t"
                  "total seconds: %s\n"
                  "\t"
                  "best perf: %llu cycles\n"
                  "\t"
                  "worst perf: %llu cycles\n"
                  "\t"
                  "avg perf: %llu cycles\n",
          c->name, backing_buffer, c->times[0], c->times[1],
          c->sum_times/c->total_times);
}

void run_benches(BenchCtx *c, s32 total_benches, s32 total_times)
{
s32 index, times, loop_counter;
s64 cycles, min_cycles, max_cycles;
BenchResult res;
  
  while(loop_counter < LOOP_AMOUNT) {
    for(index = 0; index < total_benches; index++) {
      min_cycles = max_cycles = -1;
      for(times = 0; times < total_times; times++) {
        res  = c[index].func();
        cycles = res.cycle;
        if(min_cycles == -1) min_cycles = cycles;
        if(max_cycles == -1) max_cycles = cycles;

        if(cycles < min_cycles) min_cycles = cycles;
        else if(cycles > max_cycles) max_cycles = cycles;
      }
      c[index].times[0]     = min_cycles;
      c[index].times[1]     = max_cycles; 
      c[index].sum_times   += cycles; 
      c[index].total_times++;
      c[index].total_seconds = res.clock_time;
    }
    loop_counter++;
  }
  for(index = 0; index < total_benches; index++) {
      print_bench(&c[index]);
  }
}

void run_tests()
{
  HashList hash_map;
  DataNode *data;
  ArenaAllocator *arena = arenaNew();
  hash_map = hash_list_new(arena);
  char key[] = "Content-Type";

  hash_list_push(arena,&hash_map, cstr_sized(key, sizeof(key)-1),"html/text",StringLength("html/text"));
  data = hash_list_get_data(&hash_map, "Content-Type");

  assert(data != 0);
  
  Str str = cstr_sized(data->data, data->data_size);
  if(MemoryMatch(STR_DATA(str), "html/text", STR_SIZE(str))) {
      log("hash_list_get_data, with util_node_hash function: Passed\n");
  } else {
      log("hash_list_get_data, with util_node_hash function: Failed\n");
  }
  
}

int main(int argc, char *argv[])
{
s32 total;
bool tests = false;

  if(argc > 1) {
      if(MemoryMatch(argv[1], "--tests", StringLength(argv[1]))) {
          tests = true;
      }
      else if(MemoryMatch(argv[1], "--count", StringLength(argv[1]))) {
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

  if(tests) {
    run_tests();
  } else {
    run_benches(bench_functions, StaticArraySize(bench_functions), total);
  }
  return 0;
}
