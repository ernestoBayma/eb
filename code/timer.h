#pragma once

#include "core.h"
#include <time.h>

typedef struct TimeValue TimeValue;
struct TimeValue 
{
  u64 whole_part;
  u64 remainder;
};

TimeValue timerNow();
TimeValue timerDiff(TimeValue first, TimeValue last);
void      timerAsStr(TimeValue timer, u8* backing_buffer, u64 backing_buffer_sz);
