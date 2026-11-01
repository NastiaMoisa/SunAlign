#include "calibration.h"
#include "measure.h"
#include "sunalign.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
int main(void) {
    calibration_t c; calibration_make(&c,UINT32_MAX,3049);
    assert(calibration_valid(&c));
    /* Every interrupted prefix and each individual corrupted bit must be rejected. */
    for(unsigned cut=0;cut<sizeof(c);cut++) {
        calibration_t torn; memset(&torn,255,sizeof(torn)); memcpy(&torn,&c,cut);
        assert(!calibration_valid(&torn));
    }
    for(unsigned bit=0;bit<sizeof(c)*8;bit++) {
        calibration_t bad=c; ((unsigned char *)&bad)[bit/8]^=1u<<(bit%8);
        assert(!calibration_valid(&bad));
    }
    calibration_t old,newer,broken;
    calibration_make(&old,UINT32_MAX,1200); calibration_make(&newer,0,1300);
    assert(calibration_select(&old,&newer)==&newer);
    for(unsigned cut=0;cut<sizeof(newer);cut++) {
        memset(&broken,255,sizeof(broken)); memcpy(&broken,&newer,cut);
        assert(calibration_select(&old,&broken)==&old);
    }
    newer.crc^=1; assert(calibration_select(&old,&newer)==&old);
    old.crc^=1; assert(!calibration_select(&old,&newer));
    puts("PASS two-slot recovery, CRC fallback and sequence wrap");
    measure_t sample={0}; measure_add(&sample,12); measure_add(&sample,4); measure_add(&sample,20);
    assert(sample.n==3 && sample.min==4 && sample.max==20 && sample.sum==36);
    calibration_make(&newer,4,-9001); assert(!calibration_valid(&newer));
    calibration_make(&newer,4,9001); assert(!calibration_valid(&newer));
    calibration_make(&newer,4,-9000); assert(calibration_valid(&newer));
    calibration_make(&old,5,9000); assert(calibration_select(&old,&newer)==&old);
    old.magic=0; assert(calibration_select(&old,&newer)==&newer);
    cadence_t t={.next=UINT32_MAX-2,.period=10};
    assert(!cadence_take(&t,UINT32_MAX-3)); assert(cadence_take(&t,UINT32_MAX-2));
    assert(!cadence_take(&t,6)); assert(cadence_take(&t,37));
    assert(t.missed==3 && t.late_max==30 && t.next==47);
    printf("PASS calibration: 24 interrupted prefixes, 192 bit flips rejected\n");
    printf("PASS cadence: skipped releases and uint32 wrap\n");
    return 0;
}
