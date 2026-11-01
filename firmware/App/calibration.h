#ifndef SUNALIGN_CALIBRATION_H
#define SUNALIGN_CALIBRATION_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint32_t magic,sequence; int32_t zero_cdeg; uint32_t crc; uint64_t commit; } calibration_t;
const calibration_t *calibration_select(const calibration_t *,const calibration_t *);
_Static_assert(sizeof(calibration_t)==24,"Flash record layout must be 24 bytes");
uint32_t calibration_crc(const calibration_t *);
bool calibration_valid(const calibration_t *);
void calibration_make(calibration_t *,uint32_t,int32_t);
#endif
