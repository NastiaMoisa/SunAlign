#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef struct { uint32_t CTRL; } test_systick_t;
extern test_systick_t *SysTick;
#define SysTick_CTRL_ENABLE_Msk 1u
#define SysTick_CTRL_TICKINT_Msk 2u
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t);
uint32_t HAL_GetTick(void);
#endif
