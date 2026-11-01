#include "board.h"
#include "main.h"
#include "observe.h"
#include <stdio.h>
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
static ADC_HandleTypeDef adc;
static volatile bus_status_t i2c_result=BUS_OK;
static bool i2c_active;
static uint32_t i2c_at;
static bool read_after_write;
static volatile bool receive_pending;
static uint8_t target_address, packet[8];
static uint8_t *receive_buffer;
static uint16_t receive_length;
static uint16_t ldr[2], pending_left;
static uint32_t adc_at, adc_started;
static cadence_t adc_schedule={.period=100};
uint32_t board_adc_sequence, board_adc_us;
static unsigned adc_phase;
static bool adc_ok;
static uint8_t rx;
static char commands[64];
static uint32_t command_times[64];
static volatile unsigned rx_head, rx_tail;
volatile uint32_t board_rx_lost, board_tx_lost, board_rx_errors;
uint32_t board_command_at;
static uint8_t tx[1024];
static unsigned tx_head;
static volatile unsigned tx_tail, tx_count;
static volatile bool tx_busy;
static bus_status_t start(void *ctx,bool read,uint8_t address,uint8_t reg,uint8_t *p,uint16_t len) {
    (void)ctx;
    if(i2c_active) return BUS_BUSY;
    i2c_result=BUS_PENDING; i2c_at=observe_ms(); i2c_active=true;
    read_after_write=read; receive_pending=false; target_address=address;
    receive_buffer=p; receive_length=len; packet[0]=reg;
    if(!read) for(unsigned i=0;i<len;++i) packet[i+1]=p[i];
    HAL_StatusTypeDef s=HAL_I2C_Master_Transmit_IT(&hi2c1,address<<1,packet,read ? 1 : len+1);
    if(s==HAL_OK) return BUS_OK;
    /* A busy peripheral also gets the transfer watchdog instead of retrying forever. */
    if(s==HAL_BUSY) return BUS_OK;
    i2c_active=false;
    return s==HAL_BUSY ? BUS_BUSY : BUS_ERROR;
}
static bus_status_t poll(void *ctx) {
    (void)ctx;
    if(i2c_result==BUS_PENDING && observe_ms()-i2c_at>=20) {
        /* Reset the peripheral after a stalled transfer before reusing the buffer. */
        HAL_NVIC_DisableIRQ(I2C1_IRQn);
        __HAL_RCC_I2C1_FORCE_RESET(); __HAL_RCC_I2C1_RELEASE_RESET();
        HAL_I2C_Init(&hi2c1);
        HAL_NVIC_ClearPendingIRQ(I2C1_IRQn); HAL_NVIC_EnableIRQ(I2C1_IRQn);
        i2c_result=BUS_TIMEOUT;
    }
    if(i2c_result!=BUS_PENDING) i2c_active=false;
    return i2c_result;
}
sensor_bus_t board_bus(void) { return (sensor_bus_t){NULL,start,poll}; }
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *h) { if(h==&hi2c1) i2c_result=BUS_OK; }
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *h) {
    if(h==&hi2c1) {
        if(read_after_write) receive_pending=true;
        else i2c_result=BUS_OK;
    }
}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *h) {
    if(h==&hi2c1) i2c_result=(HAL_I2C_GetError(h)&HAL_I2C_ERROR_AF) ? BUS_NACK : BUS_ERROR;
}
void I2C1_IRQHandler(void) { HAL_I2C_EV_IRQHandler(&hi2c1); HAL_I2C_ER_IRQHandler(&hi2c1); }
void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&huart2); }
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *h) {
    if(h==&huart2) { unsigned next=(rx_head+1)%64;
        if(next==rx_tail) board_rx_lost++;
        else { commands[rx_head]=(char)rx; command_times[rx_head]=observe_us(); __DMB(); rx_head=next; }
        HAL_UART_Receive_IT(&huart2,&rx,1); }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *h) {
    if(h==&huart2) { tx_tail=(tx_tail+tx_count)%sizeof(tx); tx_busy=false; }
}
#ifndef SUNALIGN_FLASH_DIAGNOSTIC
int __io_putchar(int c) {
    unsigned next=(tx_head+1)%sizeof(tx);
    if(next==tx_tail) { board_tx_lost++; return EOF; }
    tx[tx_head]=(uint8_t)c; __DMB(); tx_head=next; return c;
}
#endif
char board_command(void) {
    if(rx_head==rx_tail) return 0;
    __DMB(); char c=commands[rx_tail]; board_command_at=command_times[rx_tail];
    rx_tail=(rx_tail+1)%64; return c;
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *h) {
    if(h==&huart2) { board_rx_errors++; HAL_UART_Receive_IT(&huart2,&rx,1); }
}
static bool adc_start(unsigned channel,uint32_t now) {
    ADC_ChannelConfTypeDef ch={0};
    ch.Channel=channel ? ADC_CHANNEL_1 : ADC_CHANNEL_0;
    ch.Rank=ADC_REGULAR_RANK_1; ch.SamplingTime=ADC_SAMPLINGTIME_COMMON_1;
    if(HAL_ADC_ConfigChannel(&adc,&ch)!=HAL_OK || HAL_ADC_Start(&adc)!=HAL_OK) return false;
    adc_started=now; return true;
}
void board_init(void) {
    GPIO_InitTypeDef p={0};
    p.Pin=GPIO_PIN_0|GPIO_PIN_1; p.Mode=GPIO_MODE_ANALOG; p.Pull=GPIO_NOPULL; HAL_GPIO_Init(GPIOA,&p);
    p.Pin=GPIO_PIN_0; p.Mode=GPIO_MODE_INPUT; p.Pull=GPIO_PULLUP; HAL_GPIO_Init(GPIOB,&p);
    p.Pin=GPIO_PIN_4; p.Mode=GPIO_MODE_OUTPUT_PP; p.Pull=GPIO_NOPULL; p.Speed=GPIO_SPEED_FREQ_LOW; HAL_GPIO_Init(GPIOA,&p);
    __HAL_RCC_ADC_CLK_ENABLE();
    adc.Instance=ADC1; adc.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;
    adc.Init.Resolution=ADC_RESOLUTION_12B; adc.Init.DataAlign=ADC_DATAALIGN_RIGHT;
    adc.Init.ScanConvMode=ADC_SCAN_ENABLE; adc.Init.NbrOfConversion=1;
    adc.Init.EOCSelection=ADC_EOC_SINGLE_CONV; adc.Init.ExternalTrigConv=ADC_SOFTWARE_START;
    adc.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN;
    adc.Init.SamplingTimeCommon1=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.SamplingTimeCommon2=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.TriggerFrequencyMode=ADC_TRIGGER_FREQ_LOW;
    if(HAL_ADC_Init(&adc)!=HAL_OK || HAL_ADCEx_Calibration_Start(&adc)!=HAL_OK) Error_Handler();
    HAL_NVIC_SetPriority(I2C1_IRQn,1,0); HAL_NVIC_EnableIRQ(I2C1_IRQn);
    HAL_NVIC_SetPriority(USART2_IRQn,2,0); HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_UART_Receive_IT(&huart2,&rx,1);
}
bool board_tick(uint32_t now) {
    bool work=false;
    if(receive_pending) {
        work=true;
        receive_pending=false; read_after_write=false;
        if(HAL_I2C_Master_Receive_IT(&hi2c1,target_address<<1,receive_buffer,receive_length)!=HAL_OK)
            i2c_result=BUS_ERROR;
    }
    if(!tx_busy && tx_head!=tx_tail) {
        work=true;
        tx_count=tx_head>tx_tail ? tx_head-tx_tail : sizeof(tx)-tx_tail;
        tx_busy=true;
        if(HAL_UART_Transmit_IT(&huart2,tx+tx_tail,tx_count)!=HAL_OK) tx_busy=false;
    }
    if(adc_phase) {
        if(__HAL_ADC_GET_FLAG(&adc,ADC_FLAG_EOC)) {
            work=true;
            uint16_t value=HAL_ADC_GetValue(&adc); HAL_ADC_Stop(&adc);
            if(adc_phase==1) {
                pending_left=value;
                if(adc_start(1,now)) adc_phase=2; else { adc_phase=0; adc_ok=false; }
            } else {
                board_adc_sequence++; board_adc_us=observe_us();
                ldr[0]=pending_left; ldr[1]=value; adc_at=now; adc_ok=true; adc_phase=0;
                HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
            }
        } else if(now-adc_started>=10) {
            work=true;
            HAL_ADC_Stop(&adc); adc_ok=false; adc_phase=0;
            HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
        }
    }
    if(!adc_phase && cadence_take(&adc_schedule,now)) {
        work=true; HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
        if(adc_start(0,now)) adc_phase=1; else { adc_ok=false; HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET); }
    }
    return work;
}
panel_input_t board_inputs(const mpu6050_t *m) {
    panel_input_t in={0};
    in.imu_ok=m->online&&m->valid; in.adc_ok=adc_ok;
    in.left=ldr[0]; in.right=ldr[1]; in.imu_at=m->last_sample; in.adc_at=adc_at;
    for(unsigned i=0;i<3;++i) in.mg[i]=m->mg[i];
    in.sequence=m->sequence; in.pressed=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)==GPIO_PIN_RESET;
    return in;
}

bool board_tx_idle(void) { return !tx_busy && tx_head==tx_tail; }
