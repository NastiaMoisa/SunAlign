#include "../firmware/RTOS/runtime_clock.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(runtime_clock_value(17,0,24000,0,0,48000,48,1)==17500);
    assert(runtime_clock_value(17,1,47952,1,0,48000,48,1)==18001);
    /* Pending is still valid in the latter half of a masked tick. */
    assert(runtime_clock_value(17,1,12000,1,0,48000,48,1)==18750);
    /* Counter wrapped after the initial VAL read: use the fresh counter. */
    assert(runtime_clock_value(17,0,1,1,47904,48000,48,1)==18002);
    assert(runtime_clock_value(20,1,479952,1,0,480000,48,10)==30001);
    uint32_t before=runtime_clock_value(4294967,0,33600,0,0,48000,48,1);
    uint32_t after=runtime_clock_value(4294967,1,47952,1,0,48000,48,1);
    assert((uint32_t)(after-before)==701);
    puts("PASS RTOS clock: normal/pending/rollover/100Hz/32-bit-us wrap");
}
