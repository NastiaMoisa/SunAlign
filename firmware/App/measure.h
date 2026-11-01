#ifndef SUNALIGN_MEASURE_H
#define SUNALIGN_MEASURE_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { uint32_t n,min,max; uint64_t sum; } measure_t;
typedef struct { uint32_t next,period,missed,late_max; } cadence_t;
void measure_add(measure_t *,uint32_t);
bool cadence_take(cadence_t *,uint32_t);
#endif
