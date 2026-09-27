# SunAlign

Підказка орієнтації панелі.

Мойса Анастасія Святославівна, ІО-36.

Компоненти: MPU6050 I2C, два LDR ADC, кнопка GPIO.

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Платформа Nucleo-C031C6.

ЛР1: відтворювана Docker-збірка, UART/LED, PRD і дослідження компонентів.

`make build` збирає прошивку. Схема `diagram.json`, налаштування Wokwi `wokwi.toml`.

Документація: [PRD](docs/PRD.md), [компоненти](docs/hardware-components.md).
