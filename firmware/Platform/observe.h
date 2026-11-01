#ifndef SUNALIGN_OBSERVE_H
#define SUNALIGN_OBSERVE_H
#include "measure.h"
#include "calibration.h"
void observe_init(void);
uint32_t observe_us(void);
uint32_t observe_ms(void);
void observe_stack_paint(void);
unsigned observe_stack_used(void);
void observe_clock_report(void);
void observe_stat(const char *,const measure_t *);
bool calibration_load(calibration_t *);
bool calibration_save(int32_t);
#endif
