#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sunalign_rtos.h"
#include "board.h"
#include "observe.h"
#include "serial_text.h"
#include "main.h"
#include "build_id.h"
#include "runtime_clock.h"
#include <string.h>

extern UART_HandleTypeDef huart2;
extern uint32_t sunalign_heap_peak(void);
extern void sunalign_port_tick(void);
static TaskHandle_t hpanel, hsensor, hio;
static volatile bool kernel_started;
static QueueHandle_t input_queue, sensor_commands, zero_queue;
static measure_t stats[15], tick_cost, wake_cost;
static const char *names[] = {"board","imu","panel","led","log","loop","command","button","save","imu_dt","adc_dt","panel_dt","led_dt","log_dt","dump_wire"};
static cadence_t panel_cadence={.period=1}, led_cadence={.period=500}, log_cadence={.period=500};
static uint32_t panel_miss, led_miss, log_miss, command_miss, sensor_cmd_lost, zero_lost;
static uint32_t switches;
static volatile uint32_t wake_at;
typedef struct { uint32_t at; uint16_t l,r; int16_t pitch; } point_t;
typedef struct { panel_input_t input; int32_t zero; int16_t pitch; bool zero_set; const char *horizontal,*vertical; uint32_t calibrations, errors; } view_t;
typedef struct { uint32_t edge,accepted,count; } zero_t;
static view_t view;
static mpu6050_t imu;
static point_t history[32], snapshot[32];
static TaskStatus_t task_status[4];
/* A missed 1-tick release must not leave a high-priority task permanently Ready.
 * Skip the expired release and block until the next tick, matching Lab3's
 * no-catch-up policy. Actual missed periods remain visible in cadence metrics. */
static void periodic_wait(TickType_t *last,TickType_t period) {
    TickType_t now=xTaskGetTickCount();
    if((TickType_t)(now-*last)>=period) *last=now;
    vTaskDelayUntil(last,period);
}

uint32_t sunalign_runtime_us(void) {
    return HAL_GetTick()*1000u;
}
void sunalign_trace_in(void *task) {
    observe_task(task==hpanel?1:task==hsensor?2:task==hio?3:0);
    switches++;
}
void SysTick_Handler(void) {
    if(!kernel_started) { HAL_IncTick(); return; }
    GPIOA->BSRR=GPIO_PIN_10;
    uint32_t at=SysTick->VAL;
    for(unsigned i=0;i<1000/configTICK_RATE_HZ;i++) HAL_IncTick();
    sunalign_port_tick();
    uint32_t end=SysTick->VAL,load=SysTick->LOAD+1;
    measure_add(&tick_cost,(at>=end?at-end:at+load-end)/(SystemCoreClock/1000000u));
    GPIOA->BRR=GPIO_PIN_10;
}
void sunalign_uart_wake(void) {
    if(hio && kernel_started) {
        BaseType_t woken=pdFALSE; wake_at=sunalign_runtime_us();
        vTaskNotifyGiveFromISR(hio,&woken); portYIELD_FROM_ISR(woken);
    }
}
/* Fatal output bypasses the IRQ ring: scheduler/interrupts may be unavailable. */
static void fatal(const char *reason) {
    __disable_irq();
    while(*reason) {
        while(!(huart2.Instance->ISR&USART_ISR_TXE_TXFNF)) {}
        huart2.Instance->TDR=(uint8_t)*reason++;
    }
    for(;;) {}
}
void sunalign_assert(const char *file,unsigned line) { (void)file; (void)line; fatal("FATAL assert\r\n"); }
void vApplicationStackOverflowHook(TaskHandle_t task,char *name) { (void)task; (void)name; fatal("FATAL stack\r\n"); }
void vApplicationMallocFailedHook(void) { fatal("FATAL heap\r\n"); }

static void sensor_task(void *arg) {
    (void)arg; TickType_t last=xTaskGetTickCount(); uint32_t adc_seq=0,imu_seq=0,adc_prev=0,imu_prev=0;
    mpu6050_init(&imu,board_bus(),observe_ms());
    for(;;) {
        uint32_t t=observe_us(); board_tick(observe_ms()); measure_add(&stats[0],observe_us()-t);
        char cmd;
        if(xQueueReceive(sensor_commands,&cmd,0)==pdPASS) mpu6050_request(&imu,cmd=='n');
        t=observe_us(); mpu6050_tick(&imu,observe_ms()); measure_add(&stats[1],observe_us()-t);
        if(imu.sequence!=imu_seq) {
            if(imu_seq) measure_add(&stats[9],observe_us()-imu_prev);
            imu_seq=imu.sequence; imu_prev=observe_us();
        }
        if(board_adc_sequence!=adc_seq) {
            if(adc_seq) measure_add(&stats[10],board_adc_us-adc_prev);
            adc_seq=board_adc_sequence; adc_prev=board_adc_us;
        }
        panel_input_t in=board_inputs(&imu); xQueueOverwrite(input_queue,&in);
        /* ADC/IMU release remains100ms; 2ms service is below10/20ms watchdogs. */
        periodic_wait(&last,pdMS_TO_TICKS(2)>0?pdMS_TO_TICKS(2):1);
    }
}
static void panel_task(void *arg) {
    (void)arg; TickType_t last=xTaskGetTickCount();
    sunalign_t app; sunalign_init(&app); calibration_t saved;
    if(calibration_load(&saved)) { app.zero=saved.zero_cdeg/100.0f; app.zero_set=true; }
    uint32_t edge=0,prev=0,period=0,led_prev=0; bool previous=false;
    panel_input_t in={0};
    for(;;) {
        uint32_t now=observe_ms(),t=observe_us();
        cadence_take(&panel_cadence,now);
        xQueuePeek(input_queue,&in,0);
        /* PB0 is sampled here, independently of sensor queue freshness. */
        in.pressed=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)==GPIO_PIN_RESET;
        if(period) measure_add(&stats[11],t-period);
        period=t;
        if(in.pressed&&!previous) edge=t;
        previous=in.pressed;
        sunalign_tick(&app,&in,now);
        if(app.calibrations!=prev) {
            zero_t z={edge,observe_us(),app.calibrations}; prev=app.calibrations;
            GPIOA->ODR^=GPIO_PIN_7; measure_add(&stats[7],z.accepted-z.edge);
            if(xQueueSend(zero_queue,&z,0)!=pdPASS) zero_lost++;
        }
        view_t next={.input=in,.zero=(int32_t)(app.zero*100),.pitch=(int16_t)(app.pitch*100),
            .zero_set=app.zero_set,.horizontal=sunalign_horizontal(&app),.vertical=sunalign_vertical(&app),
            .calibrations=app.calibrations,.errors=imu.errors};
        taskENTER_CRITICAL(); view=next; taskEXIT_CRITICAL();
        measure_add(&stats[2],observe_us()-t);
        if((int32_t)(observe_ms()-panel_cadence.next)>0) panel_miss++;
        if(cadence_take(&led_cadence,now)) {
            t=observe_us(); if(led_prev) measure_add(&stats[12],t-led_prev); led_prev=t;
            HAL_GPIO_TogglePin(Led_GPIO_Port,Led_Pin); measure_add(&stats[3],observe_us()-t);
            if((uint32_t)(observe_ms()-(led_cadence.next-led_cadence.period))>5) led_miss++;
        }
        periodic_wait(&last,1);
    }
}
static view_t view_copy(void) { view_t v; taskENTER_CRITICAL(); v=view; taskEXIT_CRITICAL(); return v; }
static void os_report(void) {
    serial_text("OS_CLOCK source=HAL_ms resolution_us=1000 fine=GPIO_VCD\r\n");
    uint32_t total=0; UBaseType_t n=uxTaskGetSystemState(task_status,4,&total);
    for(unsigned i=0;i<n;i++) {
        serial_text("TASK %s %lu %u %u\r\n",task_status[i].pcTaskName,
            (unsigned long)task_status[i].ulRunTimeCounter,(unsigned)task_status[i].usStackHighWaterMark,
            (unsigned)task_status[i].uxCurrentPriority);
    }
    serial_text("OS %lu %lu %lu %lu %lu %lu\r\n",(unsigned long)total,
        (unsigned long)xPortGetFreeHeapSize(),(unsigned long)xPortGetMinimumEverFreeHeapSize(),
        (unsigned long)switches,(unsigned long)sensor_cmd_lost,(unsigned long)zero_lost);
    observe_stat("tick_cost_us",&tick_cost); observe_stat("isr_task_us",&wake_cost);
}
static void io_task(void *arg) {
    (void)arg; unsigned head=0,count=0,left=0,total=0,stat_left=0; bool exporting=false,first_adc=false,first_imu=false;
    uint32_t export_at=0,dump_start=0,log_prev=0,prev_loop=0;
    serial_text("BOOT SunAlign lab3 %s systick_1ms 48MHz 115200\r\n",BUILD_ID);
    serial_text("RTOS FreeRTOS=10.5.1 port=GCC/ARM_CM0 tick_hz=%u\r\n",configTICK_RATE_HZ);
    view_t v=view_copy(); serial_text("STORE valid=%u zero=%d\r\n",v.zero_set,(int)v.zero);
    for(;;) {
        if(wake_at) { uint32_t t=wake_at; wake_at=0; measure_add(&wake_cost,sunalign_runtime_us()-t); }
        uint32_t loop=observe_us(); if(prev_loop) measure_add(&stats[5],loop-prev_loop); prev_loop=loop;
        v=view_copy(); zero_t z;
        if(xQueueReceive(zero_queue,&z,0)==pdPASS) serial_text("ZERO %lu %lu %lu src=poll\r\n",(unsigned long)z.edge,(unsigned long)z.accepted,(unsigned long)z.count);
        if(!first_adc&&v.input.adc_ok) { first_adc=true; serial_text("FIRST adc %lu\r\n",(unsigned long)observe_us()); }
        if(!first_imu&&v.input.imu_ok) { first_imu=true; serial_text("FIRST imu %lu\r\n",(unsigned long)observe_us()); }
        char cmd=board_command();
        if(cmd) {
            uint32_t latency=observe_us()-board_command_at; measure_add(&stats[6],latency); if(latency>10000) command_miss++;
            if(cmd=='n'||cmd=='c') { if(xQueueSend(sensor_commands,&cmd,0)!=pdPASS) sensor_cmd_lost++; }
            if(cmd=='t') stat_left=19;
            if(cmd=='v') observe_clock_report();
            if(cmd=='r') NVIC_SystemReset();
            if(cmd=='o') os_report();
            if(cmd=='d') {
                total=left=count; export_at=observe_us();
                if(count&&!exporting) { exporting=true; dump_start=export_at; GPIOA->BSRR=GPIO_PIN_6; }
                for(unsigned i=0;i<count;i++) snapshot[i]=history[(head+32-count+i)%32];
            }
            if(cmd=='s') {
                uint32_t t=observe_us(); GPIOA->BSRR=GPIO_PIN_6;
                bool ok=v.zero_set&&calibration_save(v.zero);
                if(!exporting) GPIOA->BRR=GPIO_PIN_6;
                uint32_t elapsed=observe_us()-t; measure_add(&stats[8],elapsed);
                serial_text("SAVE %lu %lu %u\r\n",(unsigned long)t,(unsigned long)elapsed,ok);
            }
        }
        uint32_t now=observe_ms();
        if(cadence_take(&log_cadence,now)) {
            uint32_t t=observe_us(); if(log_prev) measure_add(&stats[13],t-log_prev); log_prev=t;
            history[head]=(point_t){now,v.input.left,v.input.right,v.pitch}; head=(head+1)%32; if(count<32) count++;
            serial_text("PANEL %lu %u %u %d %s %s %lu %lu\r\n",(unsigned long)now,v.input.left,v.input.right,(int)v.pitch,
                v.horizontal?v.horizontal:"ERROR",v.vertical?v.vertical:"ERROR",(unsigned long)v.calibrations,(unsigned long)v.errors);
            measure_add(&stats[4],observe_us()-t); if((uint32_t)(observe_ms()-(log_cadence.next-500))>20) log_miss++;
        }
        uint32_t t=observe_us();
        if(left&&(uint32_t)(t-export_at)>=1000) {
            unsigned i=total-left; serial_text("POINT %lu %u %u %d\r\n",(unsigned long)snapshot[i].at,snapshot[i].l,snapshot[i].r,snapshot[i].pitch); left--; export_at=t;
        } else if(stat_left) {
            unsigned i=19-stat_left--;
            if(i<15) { measure_t st; taskENTER_CRITICAL(); st=stats[i]; taskEXIT_CRITICAL(); observe_stat(names[i],&st); }
            if(i==15) serial_text("SCHED %lu %lu %lu %lu\r\n",(unsigned long)panel_cadence.late_max,(unsigned long)panel_cadence.missed,(unsigned long)led_cadence.late_max,(unsigned long)log_cadence.late_max);
            if(i==16) serial_text("RES %u %lu %lu %lu %lu NA\r\n",observe_stack_used(),(unsigned long)sunalign_heap_peak(),(unsigned long)board_rx_lost,(unsigned long)board_rx_errors,(unsigned long)board_tx_lost);
            if(i==17) serial_text("DEADLINE %lu %lu %lu %lu\r\n",(unsigned long)panel_miss,(unsigned long)led_miss,(unsigned long)log_miss,(unsigned long)command_miss);
            if(i==18) observe_clock_report();
        }
        if(!stat_left&&exporting&&!left&&board_tx_idle()) {
            uint32_t elapsed=observe_us()-dump_start; measure_add(&stats[14],elapsed); exporting=false; GPIOA->BRR=GPIO_PIN_6;
            serial_text("EXPORT_DONE %lu %lu\r\n",(unsigned long)dump_start,(unsigned long)elapsed);
        }
        ulTaskNotifyTake(pdTRUE,1);
    }
}
void sunalign_rtos_start(void) {
    input_queue=xQueueCreate(1,sizeof(panel_input_t)); sensor_commands=xQueueCreate(8,1); zero_queue=xQueueCreate(8,sizeof(zero_t));
    configASSERT(input_queue&&sensor_commands&&zero_queue);
    configASSERT(xTaskCreate(panel_task,"panel",192,0,4,&hpanel)==pdPASS);
    configASSERT(xTaskCreate(sensor_task,"sensors",192,0,3,&hsensor)==pdPASS);
    configASSERT(xTaskCreate(io_task,"io",256,0,2,&hio)==pdPASS);
    __disable_irq(); kernel_started=true;
    vTaskStartScheduler(); fatal("FATAL scheduler\r\n"); for(;;) {}
}
