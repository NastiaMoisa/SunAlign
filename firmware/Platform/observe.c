#include "observe.h"
#include "main.h"
#include "serial_text.h"
#include "timebase.h"
#include <string.h>
static uint32_t *paint_end;
extern uint8_t _estack;
extern uint32_t _Min_Stack_Size;
void observe_stack_paint(void) {
    uint32_t *p=(uint32_t *)((uintptr_t)&_estack-(uintptr_t)&_Min_Stack_Size);
    paint_end=(uint32_t *)__get_MSP()-16;
    while(p<paint_end) *p++=0xdeadbeefu;
}
unsigned observe_stack_used(void) {
    uint32_t *p=(uint32_t *)((uintptr_t)&_estack-(uintptr_t)&_Min_Stack_Size);
    while(p<paint_end && *p==0xdeadbeefu) p++;
    return (uintptr_t)&_estack-(uintptr_t)p;
}
void observe_init(void) {
    GPIO_InitTypeDef p={0}; p.Pin=GPIO_PIN_6|GPIO_PIN_7;
    p.Mode=GPIO_MODE_OUTPUT_PP; p.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA,&p);
}
void observe_clock_report(void) {
    timebase_report();
}
uint32_t observe_us(void) { return (uint32_t)timebase_us(); }
uint32_t observe_ms(void) { return timebase_ms(); }
void observe_stat(const char *name,const measure_t *s) {
    serial_text("STAT %s %lu %lu %lu %lu\r\n",name,
      (unsigned long)s->n,(unsigned long)s->min,
      (unsigned long)(s->n ? s->sum/s->n : 0),(unsigned long)s->max);
}
#define PAGE_A ((const calibration_t *)0x08007000u)
#define PAGE_B ((const calibration_t *)0x08007800u)
static void read_slot(const calibration_t *address,calibration_t *out) {
    const volatile uint8_t *src=(const volatile uint8_t *)address;
    uint8_t *dst=(uint8_t *)out;
    for(unsigned i=0;i<sizeof(*out);i++) dst[i]=src[i];
}
static const calibration_t *latest(void) {
    calibration_t a,b; read_slot(PAGE_A,&a); read_slot(PAGE_B,&b);
    const calibration_t *chosen=calibration_select(&a,&b);
    return chosen==&a ? PAGE_A : chosen==&b ? PAGE_B : NULL;
}
bool calibration_load(calibration_t *out) {
    const calibration_t *p=latest(); if(!p) return false;
    read_slot(p,out); return true;
}
bool calibration_save(int32_t zero) {
    const calibration_t *old=latest(), *dst=old==PAGE_A ? PAGE_B : PAGE_A;
    calibration_t r,previous;
    if(old) read_slot(old,&previous);
    calibration_make(&r,old ? previous.sequence+1u : 1u,zero);
    FLASH_EraseInitTypeDef erase={0}; uint32_t error;
    erase.TypeErase=FLASH_TYPEERASE_PAGES; erase.Page=((uintptr_t)dst-FLASH_BASE)/FLASH_PAGE_SIZE; erase.NbPages=1;
    bool ok=HAL_FLASH_Unlock()==HAL_OK;
    if(ok) ok=HAL_FLASHEx_Erase(&erase,&error)==HAL_OK;
    unsigned checked=0; uint32_t bad=0;
    const volatile uint32_t *page=(const volatile uint32_t *)dst;
    if(ok) for(;checked<FLASH_PAGE_SIZE/4;checked++) {
        bad=page[checked]; if(bad!=0xffffffffu) { ok=false; break; }
    }
    serial_text("ERASE page=%lu ff_words=%u bad=%lu\r\n",(unsigned long)erase.Page,checked,(unsigned long)bad);
    for(unsigned i=0;ok && i<3;i++) {
        uint64_t word; memcpy(&word,(const uint8_t *)&r+i*8,8);
        ok=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,(uintptr_t)dst+i*8,word)==HAL_OK;
    }
    HAL_FLASH_Lock();
    calibration_t verified; read_slot(dst,&verified);
    return ok && calibration_valid(&verified);
}
