#ifndef SUNALIGN_RUNTIME_CLOCK_H
#define SUNALIGN_RUNTIME_CLOCK_H
#include <stdint.h>
/* The caller samples pending/VAL/pending with IRQs masked, then rereads VAL if
 * rollover occurred between the first two samples. At most one tick is pending. */
static inline uint32_t runtime_clock_value(uint32_t ms,uint32_t pending_before,
    uint32_t value,uint32_t pending_after,uint32_t reloaded_value,
    uint32_t reload,uint32_t clocks_per_us,uint32_t tick_ms) {
    if(pending_after) {
        if(!pending_before) value=reloaded_value;
        ms+=tick_ms;
    }
    return ms*1000u+(reload-value)/clocks_per_us;
}
#endif
