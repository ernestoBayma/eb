#include "core.h"
#include "os.h"
#include "timer.h"

#include <stdio.h>

TimeValue timerNow()
{
  return osGetTime();
}
TimeValue timerDiff(TimeValue first, TimeValue last)
{
TimeValue result = {0};

    result.whole_part = (
        last.whole_part - first.whole_part
    );
    result.remainder = (
        last.remainder - last.remainder
    );

    return result;
}
void timerAsStr(TimeValue timer, u8* backing_buffer, u64 backing_buffer_sz)
{
  snprintf(backing_buffer, backing_buffer_sz, "%llu.%llu", timer.whole_part, timer.remainder);
}
