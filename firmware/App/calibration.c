#include "calibration.h"
uint32_t calibration_crc(const calibration_t *r) {
    const uint8_t *b=(const uint8_t *)r; uint32_t crc=~0u;
    for(unsigned n=0;n<12;n++) {
        crc^=b[n];
        for(unsigned k=0;k<8;k++) crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);
    }
    return ~crc;
}
bool calibration_valid(const calibration_t *r) {
    return r->magic==0x53414c33u && r->commit==0x314d4f43534e5553ull &&
        r->zero_cdeg>=-9000 && r->zero_cdeg<=9000 && r->crc==calibration_crc(r);
}
void calibration_make(calibration_t *r,uint32_t seq,int32_t zero) {
    r->magic=0x53414c33u; r->sequence=seq; r->zero_cdeg=zero;
    r->crc=calibration_crc(r); r->commit=0x314d4f43534e5553ull;
}

const calibration_t *calibration_select(const calibration_t *a,const calibration_t *b) {
    bool av=calibration_valid(a), bv=calibration_valid(b);
    if(av && (!bv || (int32_t)(a->sequence-b->sequence)>0)) return a;
    return bv ? b : 0;
}
