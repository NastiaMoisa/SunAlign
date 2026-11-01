#include "clock_pair.h"
#define FINE_MOD (1u<<24)
#define FINE_MASK (FINE_MOD-1u)
void clock_pair_init(clock_pair_t *s,uint32_t fine,uint16_t coarse,uint32_t cpu_hz) {
    *s=(clock_pair_t){.previous_fine=fine&FINE_MASK,.previous_coarse=coarse,
                      .cycles_per_us=cpu_hz/1000000u};
}
uint64_t clock_pair_sample(clock_pair_t *s,uint32_t fine,uint16_t coarse) {
    uint32_t ms=(uint16_t)(coarse-s->previous_coarse);
    uint32_t modulo=(s->previous_fine-fine)&FINE_MASK;
    s->previous_fine=fine&FINE_MASK; s->previous_coarse=coarse;
    s->coarse_ms+=ms; s->reads++;
    if(ms>s->max_gap_ms) s->max_gap_ms=ms;
    if(ms>60000u || !s->cycles_per_us || s->cycles_per_us>64u) s->errors|=CLOCK_PAIR_GAP;
    if(!s->errors) {
        /* A coarse millisecond bounds elapsed cycles to within one tick.
         * Nearest modular representative is unique: 2 ms << 2^24 cycles. */
        uint32_t estimate=ms*1000u*s->cycles_per_us;
        uint32_t extra=estimate>modulo ? (estimate-modulo+FINE_MOD/2u)/FINE_MOD : 0;
        uint32_t cycles=modulo+extra*FINE_MOD+s->remainder;
        s->fine_us+=cycles/s->cycles_per_us;
        s->remainder=cycles%s->cycles_per_us;
        s->extra_fine_wraps+=extra;
        uint64_t coarse_us=s->coarse_ms*1000u;
        uint64_t error=s->fine_us>coarse_us ? s->fine_us-coarse_us : coarse_us-s->fine_us;
        /* Check absolute phase, so small per-read bias cannot accumulate. */
        if(error>2000u) s->errors|=CLOCK_PAIR_PHASE;
    }
    uint64_t result=s->errors ? s->coarse_ms*1000u : s->fine_us;
    /* Recovery clock only; a latched error invalidates all CPU profiling. */
    if(result<s->returned_us) result=s->returned_us;
    s->returned_us=result;
    return result;
}
