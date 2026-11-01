#ifndef SUNALIGN_TIMEBASE_H
#define SUNALIGN_TIMEBASE_H
#include <stdbool.h>
#include <stdint.h>
void timebase_start(void);
uint64_t timebase_us(void);
uint32_t timebase_ms(void);
bool timebase_valid(void);
void timebase_report(void);
#endif
