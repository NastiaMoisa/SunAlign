# Дослідження компонентів

## Первинні джерела

1. [STM32C031x4/x6, DS13867 Rev 4](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf): с. 1, 20, 28–38; таблиці 12, 13 про корпус LQFP48, ADC й AF. Ядро Cortex-M0+, 48 МГц, Flash 32 КБ, SRAM 12 КБ.
2. [Nucleo MB1717, UM2953 Rev 2](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf): с. 14–17 про живлення, LED/VCP; с. 19–22, таблиці роз'ємів і відповідність контактів.
3. [RM0490](https://www.st.com/resource/en/reference_manual/rm0490-stm32c0-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf): розділи RCC, ADC, I2C_TIMINGR, USART_BRR. Reference manual описує блоки, а datasheet визначає допустимі піни.
4. [MPU-6000/6050 Product specification, PS-MPU-6000A-00](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf): с. 10–13, електричні характеристики; с. 21–24, I²C. VDD 2.375–3.46 В; типово 3.9 мА для accel+gyro, 500 мкА для accel. Модуль GY-521 і сам кристал мають різну обв'язку, не можна припускати однакові лінії живлення.
5. [MPU-6000/6050 Register map, RM-MPU-6000A-00 Rev 4.2](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf): с. 15, ACCEL_CONFIG 0x1C, ±2 g і 16384 LSB/g; с. 29–30, виходи акселерометра 0x3B..0x40, старший байт першим; с. 40–41, PWR_MGMT_1 0x6B; с. 45, WHO_AM_I 0x75=0x68. Старі URL TDK можуть перенаправляти на каталог виробника.
6. [Модель MPU6050 Wokwi](https://docs.wokwi.com/parts/wokwi-mpu6050): AD0=GND дає адресу 0x68, AD0=VCC дає 0x69; accelX/Y/Z задаються у g. Це документація моделі, не заміна datasheet.
7. [Модель LDR Wokwi](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor): модуль містить LDR, резистор і компаратор; AO є аналоговим виходом, DO тут не використовується. Атрибути lux, rl10, gamma визначають модель. Виробник фізичного модуля не заданий, тому точний паспортний струм, розкид і lux-калібрування не заявляються.
8. [Кнопка Wokwi](https://docs.wokwi.com/parts/wokwi-pushbutton): контакти 1.l/1.r та 2.l/2.r, модель брязкоту. Замикання на GND, внутрішній pull-up MCU.
9. [Плата Wokwi](https://docs.wokwi.com/parts/board-st-nucleo-c031c6): LED PA5, підтримка ADC/I²C/UART; DMA не моделюється.

## Інтерфейси й план живлення

| Компонент | Живлення / інтерфейс | Налаштування / призначення |
|---|---|---|
| MPU6050 | 3.3 В, GND, PB8=SCL, PB9=SDA | I²C 7-bit 0x68; HAL отримує 0xD0; standard mode близько 100 кГц. AD0=GND. Два pull-up 4.7 кОм до 3.3 В. |
| LDR left | 3.3 В, GND, AO=PA0 | ADC_IN0, 12 біт, без pull-up MCU. Датчик ліворуч від перегородки. |
| LDR right | 3.3 В, GND, AO=PA1 | ADC_IN1, 12 біт. Однакова орієнтація й геометрія з лівим. |
| Кнопка | PB0 до GND | GPIO input, pull-up MCU, debounce 30 мс. |
| LED | PA5 на платі | GPIO output; відокремлено від ADC. |
| UART | PA2 TX / PA3 RX | AF1 USART2, 115200 8N1, логічні рівні 3.3 В. |

Для прототипу закладається бюджет зовнішніх компонентів 20 мА: MPU до 4 мА типово, для кожного невизначеного LDR-модуля резерв по 5 мА, pull-up до 1.4 мА разом при LOW, решта запас. Це розрахунковий резерв, не виміряний струм. Регулятор Nucleo має також живити MCU й бортові навантаження; перед фізичним підключенням перевіряють їх суму та фактичний паспорт модулів. Немає силового навантаження. Всі землі спільні; AO не перевищує VDDA. Не живити LDR від 5 В.

Час наростання оцінюється як 0.8473·R·C. Для 4.7 кОм і 100 пФ виходить близько 398 нс, менше 1 мкс standard mode. Реальна ємність і паралельні pull-up потребують вимірювання осцилографом.
