#ifndef SUNALIGN_CLOCK_PAIR_H
#define SUNALIGN_CLOCK_PAIR_H
#include <stdbool.h>
#include <stdint.h>

/* Fine is a free-running 24-bit downcounter, coarse a 16-bit 1 kHz
 * upcounter. Coherent samples must be less than 60 seconds apart.
 * The counters share a fixed clock; no IRQ counts are needed. */
enum { CLOCK_PAIR_GAP=1u, CLOCK_PAIR_PHASE=2u, CLOCK_PAIR_CAPTURE=4u };
typedef struct {
    uint64_t coarse_ms, fine_us, returned_us;
    uint32_t previous_fine, cycles_per_us, remainder, errors, reads;
    uint32_t extra_fine_wraps, max_gap_ms;
    uint16_t previous_coarse;
} clock_pair_t;
void clock_pair_init(clock_pair_t *,uint32_t,uint16_t,uint32_t);
uint64_t clock_pair_sample(clock_pair_t *,uint32_t,uint16_t);
#endif
