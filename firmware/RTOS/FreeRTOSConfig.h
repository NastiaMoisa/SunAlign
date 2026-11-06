#ifndef SUNALIGN_FREERTOS_CONFIG_H
#define SUNALIGN_FREERTOS_CONFIG_H
#include <stdint.h>
extern uint32_t SystemCoreClock;
uint32_t sunalign_runtime_us(void);
void sunalign_trace_in(void *);
void sunalign_assert(const char *, unsigned);
#define configCPU_CLOCK_HZ SystemCoreClock
#define configTICK_RATE_HZ RTOS_TICK_HZ
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE 96
#if defined(SUNALIGN_OS_DIAGNOSTIC) && SUNALIGN_OS_DIAGNOSTIC == 5
#define configTOTAL_HEAP_SIZE 256
#else
#define configTOTAL_HEAP_SIZE 4800
#endif
#define configMAX_TASK_NAME_LEN 12
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 0
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_TIMERS 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#if defined(SUNALIGN_OS_DIAGNOSTIC) && SUNALIGN_OS_DIAGNOSTIC == 2
#define configSUPPORT_STATIC_ALLOCATION 1
#else
#define configSUPPORT_STATIC_ALLOCATION 0
#endif
#ifdef OS_STACK_CHECK
#define configCHECK_FOR_STACK_OVERFLOW OS_STACK_CHECK
#else
#define configCHECK_FOR_STACK_OVERFLOW 2
#endif
#define configUSE_MALLOC_FAILED_HOOK 1
#define configUSE_TRACE_FACILITY 1
#define configGENERATE_RUN_TIME_STATS 1
#define configRUN_TIME_COUNTER_TYPE uint32_t
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() ((void)0)
#define portGET_RUN_TIME_COUNTER_VALUE() sunalign_runtime_us()
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define configASSERT(x) do { if (!(x)) sunalign_assert(__FILE__,__LINE__); } while (0)
#define traceTASK_SWITCHED_IN() sunalign_trace_in((void *)pxCurrentTCB)
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler sunalign_port_tick
#endif
