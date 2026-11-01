#include "clock_pair.h"
#include <assert.h>
#include <stdio.h>
#define MASK 0xffffffu
static uint32_t fine_at(uint64_t us,unsigned mhz,unsigned offset) {
    return (MASK-(uint32_t)(us*mhz+offset))&MASK;
}
static uint16_t coarse_at(uint64_t us,unsigned phase,unsigned base) {
    return (uint16_t)((us+phase)/1000u+base);
}
static void trace(unsigned mhz,unsigned phase,unsigned base) {
    clock_pair_t s; const unsigned offset=9001;
    clock_pair_init(&s,fine_at(0,mhz,offset),coarse_at(0,phase,base),mhz*1000000u);
    uint64_t us=0;
    const uint32_t gaps[]={1,3,31,999,1001,65511,65536,349525,349526,1500000,50000000};
    for(unsigned i=0;i<sizeof(gaps)/sizeof(gaps[0]);i++) {
        us+=gaps[i];
        assert(clock_pair_sample(&s,fine_at(us,mhz,offset),coarse_at(us,phase,base))==us);
        assert(!s.errors);
    }
    /* Two hours, crossing many coarse wraps and the 32-bit us boundary. */
    for(unsigned i=0;i<7200;i++) {
        us+=1000000;
        uint64_t now=clock_pair_sample(&s,fine_at(us,mhz,offset),coarse_at(us,phase,base));
        assert(now==us && !s.errors);
    }
    assert(s.extra_fine_wraps>0 && s.coarse_ms>7000000);
}
static void legacy_backward_sample(void) {
    uint16_t previous=1000,read=975;
    assert((uint16_t)(read-previous)==65511); /* Observed apparent gap class. */
    clock_pair_t s;
    clock_pair_init(&s,fine_at(0,48,0),0,48000000);
    assert(clock_pair_sample(&s,fine_at(5,48,0),0)==5 && !s.errors);
    /* The replacement does not consume the ambiguous TIM3 low word. */
    uint32_t bad_fine=(s.previous_fine+25*48u)&MASK;
    (void)clock_pair_sample(&s,bad_fine,0);
    assert(s.errors&CLOCK_PAIR_PHASE); /* Fine backstep also cannot pass silently. */
    assert(clock_pair_sample(&s,fine_at(10000,48,0),10)>=5 && s.errors);
}
static void cumulative_bias(void) {
    clock_pair_t s; clock_pair_init(&s,MASK,0,48000000);
    for(unsigned i=1;i<=3000;i++) {
        /* A frequency mismatch is small per read but accumulates >2 ms. */
        (void)clock_pair_sample(&s,fine_at(i*10u,48,0),coarse_at(i*11u,0,0));
    }
    assert(s.errors&CLOCK_PAIR_PHASE);
}
static void fractional_cycles(void) {
    clock_pair_t s; const uint32_t first=5000;
    clock_pair_init(&s,first,0,48000000);
    uint64_t cycles=0; uint32_t random=1234567;
    for(unsigned i=0;i<20000;i++) {
        random=random*1664525u+1013904223u;
        cycles+=1u+random%1000000u;
        uint32_t fine=(first-(uint32_t)cycles)&MASK;
        uint16_t coarse=(uint16_t)((cycles+17000u)/48000u);
        assert(clock_pair_sample(&s,fine,coarse)==cycles/48u && !s.errors);
    }
}
static void bound(void) {
    clock_pair_t s; clock_pair_init(&s,MASK,0,48000000);
    (void)clock_pair_sample(&s,fine_at(61000000,48,0),61000);
    assert(s.errors&CLOCK_PAIR_GAP);
    /* Outside the stated domain both counters can alias simultaneously.
     * At 48 MHz their joint period is exactly 131.072 s, which no sampler
     * of only these two registers can distinguish from no elapsed time. */
    clock_pair_init(&s,MASK,0,48000000);
    assert(fine_at(131072000,48,0)==MASK && coarse_at(131072000,0,0)==0);
    assert(clock_pair_sample(&s,MASK,0)==0); /* Documented unsupported gap. */
}
int main(void) {
    for(unsigned phase=0;phase<1000;phase+=137) {
        trace(12,phase,65530); trace(48,phase,100);
    }
    legacy_backward_sample(); cumulative_bias(); fractional_cycles(); bound();
    puts("PASS clock_pair: adversarial backstep, cumulative bias, multi-wrap stalls, 2-hour traces and explicit alias bound");
}
