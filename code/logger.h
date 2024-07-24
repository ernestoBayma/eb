#if !defined(_EB_LOGGER_HEADER_)
#define _EB_LOGGER_HEADER_

#include "core.h"
#include "util.h"
#include "os.h"
#include "allocator.h"
#include "str_buf.h"

#if defined(Lang_CPP)
C_Linkage_Start
#endif

typedef struct Logger Logger;
enum LogLevel {
  Debug = 0,
  Warning,
  Error,
  Info,
  LevelCount
};

global_var Str 
LogPrefixes[] = {
  cstr_lit("[DEBUG]"),
  cstr_lit("[WARNING]"),
  cstr_lit("[ERROR]"),
  cstr_lit("[INFO]")
};

#define define_log_function(name) void name(RequestedFile file, enum LogLevel level, Str message)
typedef define_log_function(logger_func);
#define INIT_LOGGER(file) log_init((file), 0);

struct Logger 
{
    RequestedFile file;
    logger_func   *func;      
};

Logger log_init(RequestedFile file, logger_func *f);
void   log_puts_strarg(Logger l, enum LogLevel level, Str message);
void   log_puts_cstrarg(Logger l, enum LogLevel level, const char *message);
void   log_putsf(Logger l, ArenaAllocator *arena, enum LogLevel level, const char *fmt, ...); 
void   log_requested_file_error(Logger l, RequestedFile file);

#define log_puts(l, lvl, m) _Generic((m),\
  const char *: log_puts_cstrarg,\
  char *: log_puts_cstrarg,\
  Str: log_puts_strarg\
)(l,lvl,m)

#if defined(Lang_CPP)
C_Linkage_End
#endif
#endif

#if defined(EB_LOGGER_IMPLEMENTATION)
#if defined(Lang_CPP)
C_Linkage_Start
#endif

void
log_function_internal(RequestedFile file, enum LogLevel level, Str message)
{
#include <time.h>

char    time_buffer[64];
time_t  now;
StrBuf  buffer;
  
    if(level < Debug && level >= LevelCount) return;

    buffer = strbuf_alloc(.capacity=4096);
    now = time(NULL);
    strftime(time_buffer, sizeof(time_buffer), "[%Y-%m-%d %H:%M:%S] ", localtime(&now));
    strbuf_concat(&buffer, time_buffer, 0);
    strbuf_concat(&buffer, LogPrefixes[level], 0);
    strbuf_concat(&buffer, message, 0);
    strbuf_concat(&buffer, "\n", 0);
    osFullWriteToFile(file, strbuf_get_str(&buffer));
    strbuf_release(&buffer);
}

Logger 
log_init(RequestedFile rf, logger_func *f)
{
Logger l = {0};

  l.file = rf;
  if(f) 
    l.func = f;
  else  
    l.func = log_function_internal;

  return l;
}

void
log_puts_strarg(Logger l, enum LogLevel level, Str message)
{
  if(!l.func || l.file.errors != FileRequestSuccess) return;
  l.func(l.file, level, message);
}

void
log_puts_cstrarg(Logger l, enum LogLevel level, const char *message)
{
  if(!l.func || l.file.errors != FileRequestSuccess) return;
  l.func(l.file, level, cstr(message));
}

void
log_putsf(Logger l, ArenaAllocator *arena, enum LogLevel level, const char *fmt, ...)
{
#include <stdio.h>
#include <stdarg.h>

va_list         args;
char            s[1];
char            *allocated_ptr;
s32             size_needed;
Str             str;

    if(!l.func || l.file.errors != FileRequestSuccess) return;

    va_start(args, fmt);
    size_needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if(size_needed > 0) {
      allocated_ptr = (char*)push_arena_array(arena, char, size_needed + 1);
    
      va_start(args, fmt);      
      s32 written = vsnprintf(allocated_ptr, size_needed, fmt, args);
      va_end(args);

      allocated_ptr[written] = 0;
  
      str  = cstr(allocated_ptr);
      l.func(l.file, level, str);
    }
}

void 
log_requested_file_error(Logger l, RequestedFile file)
{
char buffer[64];
Str  str;

    str = cstr_static(buffer);
    humanRedableFileRequestError(str, file.errors);
    log_puts(l, Error, str); 
}

#if defined(Lang_CPP)
C_Linkage_End
#endif

#endif
