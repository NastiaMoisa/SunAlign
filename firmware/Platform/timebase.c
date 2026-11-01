#include "timebase.h"
#include "main.h"
#include "serial_text.h"

/* HAL_InitTick configures SysTick and its interrupt. Do not reset it here. */
void timebase_start(void) { }

uint32_t timebase_ms(void) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uint32_t tick = HAL_GetTick();
    __set_PRIMASK(mask);
    return tick;
}

uint64_t timebase_us(void) {
    return (uint64_t)timebase_ms() * 1000u;
}

/* Configuration only; this does not prove progress while IRQs are masked. */
bool timebase_valid(void) {
    return (SysTick->CTRL & (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk)) ==
           (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
}

void timebase_report(void) {
    serial_text("CLOCK mode=systick_ms resolution_us=1000 configured=%u wcet=unavailable cpu=unavailable masked_ticks=may_be_lost\r\n",
                timebase_valid());
}
