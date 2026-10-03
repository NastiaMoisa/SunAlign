# Одночасне підключення

Плата NUCLEO-C031C6, STM32C031C6T6, LQFP48. Джерела: [DS13867 Rev 4, таблиці 12–13, с. 30–38](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf), [UM2953 Rev 2, с. 19–22, таблиці роз'ємів](https://www.st.com/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf). Дані про AF не запозичені з F411/F446.

| MCU | AF / блок | Роз'єм плати | Компонент | Рівень / підтягування | Чому цей пін |
|---|---|---|---|---|---|
| PB8 | AF6 I2C1 SCL | CN5-10 D15 | MPU SCL, analyzer D0 | 3.3 В, зовнішній 4.7 кОм | Виділений I²C контакт Arduino |
| PB9 | AF6 I2C1 SDA | CN5-9 D14 | MPU SDA, analyzer D1 | 3.3 В, зовнішній 4.7 кОм | Пара з PB8, без перетину ADC |
| PA0 | analog ADC1 IN0 | CN8-1 A0 | LDR left AO | 0..VDDA, без pull | Окремий аналоговий канал |
| PA1 | analog ADC1 IN1 | CN8-2 A1 | LDR right AO | 0..VDDA, без pull | Сусідній незалежний ADC канал |
| PB0 | GPIO input | CN5-3 D10 | ZERO до GND, analyzer D4 | 3.3 В, pull-up MCU | Не займає UART, LED чи SWD |
| PA4 | GPIO output | CN8-3 A2 | Analyzer D3 | 3.3 В, push-pull | Маркер пари ADC без зміни сенсорних входів |
| PA5 | GPIO output | CN5-6 D13 / LD4 | LED, analyzer D5 | 3.3 В, бортовий резистор | Уже встановлений LED |
| PA2 | AF1 USART2 TX | CN7 morpho / USART2 VCP | Serial RX, analyzer D2 | 3.3 В, без pull | Збережений UART шаблону |
| PA3 | AF1 USART2 RX | CN7 morpho / USART2 VCP | Serial TX | 3.3 В, без pull | Команди c/n |
| 3V3 / GND | power | CN6 живлення | MPU, LDR, pull-up, analyzer GND | Спільна земля | AO не може бути 5 В |

У схемі Wokwi використовуються імена MCU, а не неоднозначні D0/D1. Фізичну маршрутизацію VCP та перемички звіряють з UM2953 для конкретної ревізії плати. AD0=GND, адреса MPU 0x68; WHO_AM_I 0x75=0x68. Адреса 0x69 навмисно відсутня і служить контрольованим fault injection. LDR і кнопка не мають цифрового ID.

HSI=48 МГц, AHB=APB=48 МГц. UART BRR округлюється до 417, 48e6/417=115107.9 бод, похибка −0.08%. I²C TIMINGR=0x20303E5D: PRESC=2, SCLL=93, SCLH=62. Базовий період без затримок фронтів/фільтра дорівнює (94+63)·3/48e6=9.8125 мкс, приблизно 101.91 кГц. Фактичний період визначається capture. ADC PCLK/4=12 МГц, sampling=160.5 cycles, 12-bit conversion ще 12.5 cycles: близько 14.42 мкс на канал без програмних затримок.

Відхилені варіанти: PA2/PA3 для LDR конфліктують з UART; PA5 як ADC конфліктує з LED; PB8/PB9 як аналогові входи чи A4/A5 у схемі дублюють I²C через перемички MB1717. PA13/PA14 лишаються SWD. DMA не використовується і конфліктів DMA-запитів немає. Камери, SPI і PWM-приводу у продукті немає. Бюджет живлення й pull-up розраховані в hardware-components.md. В Wokwi не перевіряються струм, шум ADC, фронти й паразитна ємність.
