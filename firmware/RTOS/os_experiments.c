#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "main.h"
#include "serial_text.h"
#include "measure.h"
#include "board.h"
#include "sunalign_rtos.h"
#include "build_id.h"
#include "runtime_clock.h"
extern UART_HandleTypeDef huart2;
extern void sunalign_port_tick(void);
static TaskHandle_t high_handle,medium_handle;
static volatile bool kernel_started;
static SemaphoreHandle_t resource;
static QueueHandle_t events;
static uint32_t switches,low_count,received,dropped,accepted,timeouts,wait_begin;
static measure_t tick_cost,context_cost,isr_cost;
static TaskStatus_t task_status[5];
static bool from_isr;
uint32_t sunalign_runtime_us(void) {
    return HAL_GetTick()*1000u;
}
void sunalign_trace_in(void *task) { (void)task; switches++; }
void SysTick_Handler(void) {
    if(!kernel_started) { HAL_IncTick(); return; }
    uint32_t t=SysTick->VAL;
    for(unsigned i=0;i<1000/configTICK_RATE_HZ;i++) HAL_IncTick();
    sunalign_port_tick();
    uint32_t end=SysTick->VAL,load=SysTick->LOAD+1;
    measure_add(&tick_cost,(t>=end?t-end:t+load-end)/(SystemCoreClock/1000000u));
}
void sunalign_uart_wake(void) { }
static void fatal(const char *s) {
    __disable_irq(); while(*s) { while(!(huart2.Instance->ISR&USART_ISR_TXE_TXFNF)) {} huart2.Instance->TDR=(uint8_t)*s++; }
    for(;;) {}
}
void sunalign_assert(const char *f,unsigned n) { (void)f;(void)n;fatal("FATAL assert\r\n"); }
void vApplicationMallocFailedHook(void) { fatal("FATAL heap\r\n"); }
void vApplicationStackOverflowHook(TaskHandle_t t,char *name) { (void)t;(void)name;fatal("FATAL stack\r\n"); }
static void summary(void) {
    serial_text("OS_CLOCK source=HAL_ms resolution_us=1000 fine=GPIO_VCD\r\n");
    uint32_t total=0; unsigned n=uxTaskGetSystemState(task_status,5,&total);
    for(unsigned i=0;i<n;i++) serial_text("TASK %s %lu %u %u\r\n",task_status[i].pcTaskName,(unsigned long)task_status[i].ulRunTimeCounter,(unsigned)task_status[i].usStackHighWaterMark,(unsigned)task_status[i].uxCurrentPriority);
    serial_text("OS total_us=%lu switches=%lu heap_free=%lu heap_min=%lu low=%lu accepted=%lu received=%lu dropped=%lu timeouts=%lu\r\n",
        (unsigned long)total,(unsigned long)switches,(unsigned long)xPortGetFreeHeapSize(),(unsigned long)xPortGetMinimumEverFreeHeapSize(),
        (unsigned long)low_count,(unsigned long)accepted,(unsigned long)received,(unsigned long)dropped,(unsigned long)timeouts);
    serial_text("RX lost=%lu errors=%lu\r\n",(unsigned long)board_rx_lost,(unsigned long)board_rx_errors);
    serial_text("COST tick %lu %lu %lu %lu\r\n",(unsigned long)tick_cost.n,(unsigned long)tick_cost.min,(unsigned long)(tick_cost.n?tick_cost.sum/tick_cost.n:0),(unsigned long)tick_cost.max);
    serial_text("COST context %lu %lu %lu %lu\r\n",(unsigned long)context_cost.n,(unsigned long)context_cost.min,(unsigned long)(context_cost.n?context_cost.sum/context_cost.n:0),(unsigned long)context_cost.max);
    serial_text("COST isr %lu %lu %lu %lu\r\n",(unsigned long)isr_cost.n,(unsigned long)isr_cost.min,(unsigned long)(isr_cost.n?isr_cost.sum/isr_cost.n:0),(unsigned long)isr_cost.max);
}
static void burn(uint32_t us) { uint32_t t=sunalign_runtime_us(); while(sunalign_runtime_us()-t<us) __NOP(); }
#if SUNALIGN_OS_DIAGNOSTIC == 1
static void high_task(void *arg) {
    (void)arg;
    for(;;) {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        xSemaphoreTake(resource,portMAX_DELAY);
        serial_text("INVERSION %lu\r\n",(unsigned long)(sunalign_runtime_us()-wait_begin));
        xSemaphoreGive(resource);
    }
}
static void medium_task(void *arg) { (void)arg;for(;;) { ulTaskNotifyTake(pdTRUE,portMAX_DELAY);burn(80000); } }
static void low_task(void *arg) {
    (void)arg;
    for(unsigned mode=0;mode<2;mode++) {
        resource=mode?xSemaphoreCreateBinary():xSemaphoreCreateMutex();
        if(mode) xSemaphoreGive(resource);
        serial_text("MODE %s\r\n",mode?"binary":"mutex");
        for(unsigned i=0;i<30;i++) {
            xSemaphoreTake(resource,portMAX_DELAY);
            wait_begin=sunalign_runtime_us();xTaskNotifyGive(high_handle);
            xTaskNotifyGive(medium_handle);burn(25000);xSemaphoreGive(resource);
            vTaskDelay(pdMS_TO_TICKS(150)); low_count++;
        }
        vSemaphoreDelete(resource);
    }
    summary();serial_text("DONE inversion\r\n");vTaskSuspend(NULL);
}
#elif SUNALIGN_OS_DIAGNOSTIC == 2
#ifndef OS_STACK_WORDS
#define OS_STACK_WORDS 64
#endif
static StaticTask_t overflow_tcb,idle_tcb;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
/* Controlled adjacent storage makes the unchecked overflow observable without
 * letting this deliberate fault overwrite another task's TCB. */
static struct { StackType_t guard[128]; StackType_t stack[OS_STACK_WORDS]; } overflow_memory;
void vApplicationGetIdleTaskMemory(StaticTask_t **t,StackType_t **s,uint32_t *n) {
    *t=&idle_tcb;*s=idle_stack;*n=configMINIMAL_STACK_SIZE;
}
static void descend(unsigned depth) __attribute__((noinline));
static void descend(unsigned depth) {
    volatile uint32_t pad[24];for(unsigned i=0;i<24;i++) pad[i]=i+depth;
    if(depth) descend(depth-1);else vTaskDelay(1);
    if(pad[3]==0xffffffffu) serial_text("UNEXPECTED\r\n");
}
static void low_task(void *arg) {
    (void)arg;serial_text("STACK words=%u check=%u\r\n",OS_STACK_WORDS,configCHECK_FOR_STACK_OVERFLOW);descend(3);
    serial_text("STACK_WATERMARK free_words=%u total_words=%u\r\n",(unsigned)uxTaskGetStackHighWaterMark(NULL),OS_STACK_WORDS);
    unsigned corrupt=0;for(unsigned i=0;i<128;i++) if(overflow_memory.guard[i]!=0x11223344u) corrupt++;
    serial_text("SURVIVED check=%u guard_corrupt_words=%u\r\n",configCHECK_FOR_STACK_OVERFLOW,corrupt);
    for(;;)vTaskDelay(1);
}
#elif SUNALIGN_OS_DIAGNOSTIC == 3
static void low_task(void *arg) { (void)arg;for(;;) { low_count++;vTaskDelay(1); } }
static void high_task(void *arg) { (void)arg;vTaskDelay(50);serial_text("STARVE begin\r\n");burn(5000000);summary();serial_text("STARVE end\r\n");vTaskDelay(100);summary();serial_text("DONE starvation\r\n");vTaskSuspend(NULL); }
#elif SUNALIGN_OS_DIAGNOSTIC == 4
static void low_task(void *arg) { (void)arg;uint32_t v;for(;;) { if(xQueueReceive(events,&v,portMAX_DELAY)==pdPASS) received++;vTaskDelay(10); } }
static void high_task(void *arg) {
    (void)arg;
    for(unsigned trial=0;trial<30;trial++) {
        for(uint32_t i=0;i<100;i++) { if(xQueueSend(events,&i,0)==pdPASS) accepted++;else dropped++; }
        uint32_t v=0; if(xQueueSend(events,&v,1)==pdPASS) accepted++;else timeouts++;
        vTaskDelay(100);
    }
    summary();serial_text("DONE queue\r\n");vTaskSuspend(NULL);
}
#elif SUNALIGN_OS_DIAGNOSTIC == 6
void EXTI0_1_IRQHandler(void) {
    BaseType_t woken=pdFALSE; __HAL_GPIO_EXTI_CLEAR_FALLING_IT(GPIO_PIN_0);
    GPIOA->BSRR=GPIO_PIN_7;wait_begin=sunalign_runtime_us();from_isr=true;
    vTaskNotifyGiveFromISR(high_handle,&woken);portYIELD_FROM_ISR(woken);
}
static void high_task(void *arg) {
    (void)arg;
    for(;;) { ulTaskNotifyTake(pdTRUE,portMAX_DELAY);GPIOA->BRR=from_isr?GPIO_PIN_7:GPIO_PIN_6;measure_add(from_isr?&isr_cost:&context_cost,sunalign_runtime_us()-wait_begin); }
}
static void low_task(void *arg) {
    (void)arg;
    for(unsigned i=0;i<100;i++) {
        from_isr=false;wait_begin=sunalign_runtime_us();GPIOA->BSRR=GPIO_PIN_6;xTaskNotifyGive(high_handle);vTaskDelay(1);
        NVIC_SetPendingIRQ(EXTI0_1_IRQn);vTaskDelay(1);
    }
    summary();serial_text("DONE costs\r\n");vTaskSuspend(NULL);
}
#endif
void sunalign_rtos_start(void) {
    serial_text("BOOT SunAlign OS_DIAG %u %s\r\n",SUNALIGN_OS_DIAGNOSTIC,BUILD_ID);
#if SUNALIGN_OS_DIAGNOSTIC == 5
    (void)xTaskCreate((TaskFunction_t)sunalign_rtos_start,"exhaust",256,0,1,NULL);
    fatal("UNEXPECTED heap success\r\n");
#else
#if SUNALIGN_OS_DIAGNOSTIC == 2
    for(unsigned i=0;i<128;i++)overflow_memory.guard[i]=0x11223344u;
    configASSERT(xTaskCreateStatic(low_task,"low",OS_STACK_WORDS,0,1,overflow_memory.stack,&overflow_tcb));
#else
    xTaskCreate(low_task,"low",256,0,1,NULL);
#endif
#if SUNALIGN_OS_DIAGNOSTIC != 2
    xTaskCreate(high_task,"high",256,0,4,&high_handle);
#endif
#if SUNALIGN_OS_DIAGNOSTIC == 1
    xTaskCreate(medium_task,"medium",128,0,3,&medium_handle);
#elif SUNALIGN_OS_DIAGNOSTIC == 4
    events=xQueueCreate(8,sizeof(uint32_t));configASSERT(events);
#elif SUNALIGN_OS_DIAGNOSTIC == 6
    HAL_NVIC_SetPriority(EXTI0_1_IRQn,2,0);HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
#endif
    __disable_irq();kernel_started=true;
    vTaskStartScheduler();fatal("FATAL scheduler\r\n");
#endif
    for(;;) {}
}
