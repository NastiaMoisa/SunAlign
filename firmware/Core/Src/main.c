/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "serial_text.h"
#include "board.h"
#include "observe.h"
#include "build_id.h"
#ifdef SUNALIGN_FLASH_DIAGNOSTIC
#include "flash_probe.h"
#endif
#include <string.h>
extern uint32_t sunalign_heap_peak(void);

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Nonblocking UART transport is owned by Platform/board.c. */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  observe_stack_paint();
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  observe_init();
#ifdef SUNALIGN_FLASH_DIAGNOSTIC
  flash_diagnostic_run();
#endif
  board_init();
  static mpu6050_t imu;
  static sunalign_t app;
  static measure_t stats[15];
  static const char *names[]={"board","imu","panel","led","log","loop","command","button","save","imu_dt","adc_dt","panel_dt","led_dt","log_dt","dump_wire"};
  static cadence_t panel={.period=1}, led={.period=500}, log={.period=500};
  typedef struct { uint32_t at; uint16_t l,r; int16_t pitch; } point_t;
  static point_t history[32], snapshot[32];
  unsigned hist_head=0, hist_count=0, export_left=0, export_total=0, stat_left=0;
  uint32_t button_edge=0, prev_cal=0, export_at=0;
  bool first_adc=false, first_imu=false,button_previous=false,exporting=false;
  uint32_t dump_start=0;
  uint32_t prev_period[5]={0}, adc_seq=0, imu_seq=0;
  uint32_t panel_end_miss=0,led_end_miss=0,log_end_miss=0,command_miss=0;
  mpu6050_init(&imu, board_bus(), observe_ms());
  sunalign_init(&app);
  calibration_t saved;
  if(calibration_load(&saved)) { app.zero=saved.zero_cdeg/100.0f; app.zero_set=true; }
  serial_text("BOOT SunAlign lab3 %s systick_1ms 48MHz 115200\r\n",BUILD_ID);
  serial_text("STORE valid=%u zero=%d\r\n",app.zero_set,(int)(app.zero*100));
  uint32_t prev_loop=0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t loop_start=observe_us(), t=loop_start, now=observe_ms();
    if(prev_loop) measure_add(&stats[5],loop_start-prev_loop);
    prev_loop=loop_start;
    bool board_work=board_tick(now);
    uint32_t end=observe_us(); measure_add(&stats[0],end-t); (void)board_work;
    char cmd=board_command();
    if(cmd) {
      uint32_t latency=observe_us()-board_command_at;
      measure_add(&stats[6],latency); if(latency>10000u) command_miss++;
      if(cmd=='n'||cmd=='c') mpu6050_request(&imu,cmd=='n');
      if(cmd=='t') stat_left=19;
      if(cmd=='v') observe_clock_report();
      if(cmd=='r') NVIC_SystemReset();
      if(cmd=='d') {
        export_total=export_left=hist_count; export_at=observe_us();
        if(hist_count && !exporting) { exporting=true; dump_start=export_at; GPIOA->BSRR=GPIO_PIN_6; }
        for(unsigned i=0;i<hist_count;i++) snapshot[i]=history[(hist_head+32-hist_count+i)%32];
      }
      if(cmd=='s') {
        t=observe_us(); GPIOA->BSRR=GPIO_PIN_6;
        bool ok=app.zero_set && calibration_save((int32_t)(app.zero*100));
        if(!exporting) GPIOA->BRR=GPIO_PIN_6;
        uint32_t elapsed=observe_us()-t;
        measure_add(&stats[8],elapsed);
        serial_text("SAVE %lu %lu %u\r\n",(unsigned long)t,(unsigned long)elapsed,ok);
      }
    }
    now=observe_ms();
    t=observe_us(); unsigned old_state=imu.state; bool old_wait=imu.waiting;
    mpu6050_tick(&imu,now); end=observe_us();
    measure_add(&stats[1],end-t); (void)old_state; (void)old_wait;
    if(imu.sequence!=imu_seq) {
      if(imu_seq) measure_add(&stats[9],end-prev_period[0]);
      imu_seq=imu.sequence; prev_period[0]=end;
    }
    if(board_adc_sequence!=adc_seq) {
      if(adc_seq) measure_add(&stats[10],board_adc_us-prev_period[1]);
      adc_seq=board_adc_sequence; prev_period[1]=board_adc_us;
    }
    if(cadence_take(&panel,now)) {
      t=observe_us(); panel_input_t in=board_inputs(&imu);
      if(prev_period[2]) measure_add(&stats[11],t-prev_period[2]);
      prev_period[2]=t;
      if(in.pressed && !button_previous) button_edge=t;
      button_previous=in.pressed;
      sunalign_tick(&app,&in,now);
      if(app.calibrations!=prev_cal) {
        uint32_t accepted=observe_us(); measure_add(&stats[7],accepted-button_edge);
        GPIOA->ODR^=GPIO_PIN_7; prev_cal=app.calibrations;
        serial_text("ZERO %lu %lu %lu src=poll\r\n",(unsigned long)button_edge,(unsigned long)accepted,(unsigned long)prev_cal);
      }
      if(!first_adc && in.adc_ok) { first_adc=true; serial_text("FIRST adc %lu\r\n",(unsigned long)observe_us()); }
      if(!first_imu && in.imu_ok) { first_imu=true; serial_text("FIRST imu %lu\r\n",(unsigned long)observe_us()); }
      end=observe_us(); measure_add(&stats[2],end-t);
      if((int32_t)(observe_ms()-panel.next)>0) panel_end_miss++;
    }
    if(cadence_take(&led,now)) {
      t=observe_us();
      if(prev_period[3]) measure_add(&stats[12],t-prev_period[3]);
      prev_period[3]=t;
      HAL_GPIO_TogglePin(Led_GPIO_Port,Led_Pin);
      end=observe_us(); measure_add(&stats[3],end-t);
      if((uint32_t)(observe_ms()-(led.next-led.period))>5) led_end_miss++;
    }
    if(cadence_take(&log,now)) {
      t=observe_us(); panel_input_t in=board_inputs(&imu);
      if(prev_period[4]) measure_add(&stats[13],t-prev_period[4]);
      prev_period[4]=t;
      history[hist_head].at=now; history[hist_head].l=in.left; history[hist_head].r=in.right;
      history[hist_head].pitch=(int16_t)(app.pitch*100);
      hist_head=(hist_head+1)%32; if(hist_count<32) hist_count++;
      serial_text("PANEL %lu %u %u %d %s %s %lu %lu\r\n",
          (unsigned long)now,in.left,in.right,(int)(app.pitch*100),sunalign_horizontal(&app),sunalign_vertical(&app),
          (unsigned long)app.calibrations,(unsigned long)imu.errors);
      end=observe_us(); measure_add(&stats[4],end-t);
      if((uint32_t)(observe_ms()-(log.next-log.period))>20) log_end_miss++;
    }
    t=observe_us();
    if(export_left && (uint32_t)(t-export_at)>=1000) {
      unsigned i=export_total-export_left;
      serial_text("POINT %lu %u %u %d\r\n",(unsigned long)snapshot[i].at,snapshot[i].l,snapshot[i].r,snapshot[i].pitch);
      export_left--; export_at=t;
    } else if(stat_left) {
      unsigned i=19-stat_left--;
      if(i<15) observe_stat(names[i],&stats[i]);
      if(i==15) serial_text("SCHED %lu %lu %lu %lu\r\n",
        (unsigned long)panel.late_max,(unsigned long)panel.missed,(unsigned long)led.late_max,(unsigned long)log.late_max);
      if(i==16) serial_text("RES %u %lu %lu %lu %lu NA\r\n",
        observe_stack_used(),(unsigned long)sunalign_heap_peak(),(unsigned long)board_rx_lost,(unsigned long)board_rx_errors,(unsigned long)board_tx_lost);
      if(i==17) serial_text("DEADLINE %lu %lu %lu %lu\r\n",(unsigned long)panel_end_miss,
        (unsigned long)led_end_miss,(unsigned long)log_end_miss,(unsigned long)command_miss);
      if(i==18) observe_clock_report();
    }
    if(stat_left==0 && exporting && !export_left && board_tx_idle()) {
      uint32_t elapsed=observe_us()-dump_start;
      measure_add(&stats[14],elapsed); exporting=false; GPIOA->BRR=GPIO_PIN_6;
      serial_text("EXPORT_DONE %lu %lu\r\n",(unsigned long)dump_start,(unsigned long)elapsed);
    }


  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x20303E5D;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : Led_Pin */
  GPIO_InitStruct.Pin = Led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(Led_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: serial_text("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
