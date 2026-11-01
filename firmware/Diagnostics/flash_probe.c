#include "flash_probe.h"
#include "main.h"
#include "build_id.h"
#include "calibration.h"
#include "serial_text.h"
#include "timebase.h"
#include <stdio.h>
#include <string.h>
extern UART_HandleTypeDef huart2;
static uint32_t uart_failures;
int __io_putchar(int c) {
    uint8_t byte=(uint8_t)c;
    if(HAL_UART_Transmit(&huart2,&byte,1,1000)!=HAL_OK) { uart_failures++; return EOF; }
    return c;
}
static void status(const char *stage,int result,uint32_t page_error) {
    /* Capture before printing: HAL may clear SR error bits, ErrorCode is separate. */
    uint32_t sr=FLASH->SR,cr=FLASH->CR,error=HAL_FLASH_GetError();
    serial_text("FLASH_STATUS stage=%s result=%d error=0x%lx sr=0x%lx cr=0x%lx page_error=0x%lx tick=%lu clock_valid=%u\r\n",
      stage,result,(unsigned long)error,(unsigned long)sr,(unsigned long)cr,
      (unsigned long)page_error,(unsigned long)HAL_GetTick(),timebase_valid());
}
static bool scan(unsigned page,const char *stage) {
    const volatile uint32_t *p=(const volatile uint32_t *)(FLASH_BASE+page*FLASH_PAGE_SIZE);
    unsigned ff=0,non_ff=0,first=FLASH_PAGE_SIZE/4;
    uint32_t first_value=0,last_value=0;
    for(unsigned i=0;i<FLASH_PAGE_SIZE/4;i++) {
        uint32_t value=p[i]; last_value=value;
        if(value==0xffffffffu) ff++;
        else { non_ff++; if(first==FLASH_PAGE_SIZE/4) { first=i; first_value=value; } }
    }
    serial_text("FLASH_SCAN stage=%s page=%u words=%u ff=%u nonff=%u first_nonff=%u value=0x%lx last=0x%lx\r\n",
      stage,page,FLASH_PAGE_SIZE/4,ff,non_ff,first,(unsigned long)first_value,(unsigned long)last_value);
    return non_ff==0;
}
static bool program_record(unsigned page,unsigned sequence,int zero) {
    calibration_t record; calibration_make(&record,sequence,zero);
    uintptr_t address=FLASH_BASE+page*FLASH_PAGE_SIZE;
    for(unsigned i=0;i<sizeof(record)/8;i++) {
        uint64_t word; memcpy(&word,(const uint8_t *)&record+i*8,8);
        HAL_StatusTypeDef result=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,address+i*8,word);
        status(i==2 ? "program_commit" : "program_payload",result,0xffffffffu);
        serial_text("FLASH_WORD page=%u index=%u low=0x%lx high=0x%lx\r\n",page,i,
          (unsigned long)(uint32_t)word,(unsigned long)(uint32_t)(word>>32));
        if(result!=HAL_OK) return false;
    }
    calibration_t readback;
    for(unsigned i=0;i<sizeof(readback);i++)
        ((uint8_t *)&readback)[i]=((const volatile uint8_t *)address)[i];
    bool bytes=memcmp(&record,&readback,sizeof(record))==0;
    bool valid=calibration_valid(&readback);
    serial_text("FLASH_VERIFY page=%u bytes_equal=%u crc_commit_valid=%u sequence=%lu zero=%ld\r\n",
      page,bytes,valid,(unsigned long)readback.sequence,(long)readback.zero_cdeg);
    return bytes && valid;
}
void flash_diagnostic_run(void) {
    serial_text("BOOT SunAlign FLASH_DIAG %s HAL_OK=0 ERROR=1 BUSY=2 TIMEOUT=3 SKIP=-1\r\n",BUILD_ID);
    serial_text("FLASH_LAYOUT base=0x%lx size_kib=%u page_bytes=%u target_pages=14,15\r\n",
      (unsigned long)FLASH_BASE,(unsigned)(FLASH_SIZE/1024u),FLASH_PAGE_SIZE);
    /* Check that both time sources advance before calling timeout-based HAL. */
    uint32_t start=(uint32_t)timebase_us();
    unsigned spins=0;
    while((uint32_t)((uint32_t)timebase_us()-start)<10000u && timebase_valid() && spins++<50000u) { }
    timebase_report();
    if(!timebase_valid() || HAL_GetTick()<5u || spins>=50000u) {
        serial_text("FLASH_DIAG_ABORT clock_not_valid; no erase_or_program_called\r\n");
        for(;;) __NOP();
    }
    status("before",-1,0xccccccccu);
    scan(14,"before"); scan(15,"before");
    HAL_StatusTypeDef unlock=HAL_FLASH_Unlock(); status("unlock",unlock,0xccccccccu);
    scan(14,"after_unlock"); scan(15,"after_unlock");
    bool verified[2]={false,false};
    for(unsigned slot=0;slot<2;slot++) {
        unsigned page=14+slot; uint32_t page_error=0xccccccccu;
        int erased=-1;
        if(unlock==HAL_OK) {
            FLASH_EraseInitTypeDef request={.TypeErase=FLASH_TYPEERASE_PAGES,.Page=page,.NbPages=1};
            erased=HAL_FLASHEx_Erase(&request,&page_error);
        }
        status(slot ? "erase_B" : "erase_A",erased,page_error);
        bool all_ff=scan(page,"after_erase");
        scan(29-page,"other_after_erase");
        if(erased==HAL_OK && all_ff) verified[slot]=program_record(page,slot+1,slot ? -5678 : 1234);
        else serial_text("FLASH_PROGRAM_SKIP page=%u erase_result=%d all_ff=%u\r\n",page,erased,all_ff);
        scan(14,"after_program_phase"); scan(15,"after_program_phase");
    }
    HAL_StatusTypeDef lock=HAL_FLASH_Lock(); status("lock",lock,0xccccccccu);
    scan(14,"final"); scan(15,"final"); timebase_report();
    serial_text("FLASH_DIAG_DONE verified_A=%u verified_B=%u lock=%u uart_failures=%lu\r\n",
      verified[0],verified[1],lock,(unsigned long)uart_failures);
    for(;;) __NOP();
}
