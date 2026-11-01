#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "timebase.h"

static uint32_t mask, tick;
static test_systick_t systick;
test_systick_t *SysTick = &systick;
uint32_t __get_PRIMASK(void) { return mask; }
void __disable_irq(void) { mask = 1; }
void __set_PRIMASK(uint32_t value) { mask = value; }
uint32_t HAL_GetTick(void) { assert(mask == 1); return tick; }
void serial_text(const char *format, ...) { (void)format; }

int main(void) {
    systick.CTRL = 3;
    timebase_start();
    assert(timebase_valid());
    systick.CTRL = 1;
    assert(!timebase_valid());
    systick.CTRL = 3;
    tick = 123; mask = 0;
    assert(timebase_ms() == 123 && mask == 0);
    mask = 1;
    assert(timebase_us() == 123000 && mask == 1);
    tick = UINT32_MAX;
    uint32_t a = (uint32_t)timebase_us();
    tick = 0;
    uint32_t b = (uint32_t)timebase_us();
    assert((uint32_t)(b - a) == 1000);
    tick = UINT32_MAX;
    a = timebase_ms();
    tick = 0;
    assert((uint32_t)(timebase_ms() - a) == 1);
    puts("PASS timebase: configuration, millisecond and microsecond wrap, IRQ mask restore");
}
