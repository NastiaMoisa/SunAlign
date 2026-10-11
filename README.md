# SunAlign

Підказка орієнтації панелі.

Мойса Анастасія Святославівна, ІО-36.

Компоненти: MPU6050 I2C, два LDR ADC, кнопка GPIO.

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Платформа Nucleo-C031C6.

ЛР1: відтворювана Docker-збірка, UART/LED, PRD і дослідження компонентів. Стан збережено тегом `lab1`.

ЛР2: MPU6050, два LDR та ZERO працюють разом. LED дає heartbeat, UART показує LEFT/RIGHT, LOWER/RAISE, DARK або ERROR. Нуль задається кнопкою й живе до reset. LDR не є повіреним люксметром.

`make build` збирає прошивку. Схема `diagram.json`, налаштування Wokwi `wokwi.toml`.

Документація: [PRD](docs/PRD.md), [компоненти](docs/hardware-components.md).

`make test` запускає host-тести й перевірку залежностей, `make coverage` вимірює покриття. У Serial Monitor команда `c` повторює ідентифікацію, `n` перевіряє відсутню адресу 0x69. Використовувати наявну ліцензію Wokwi.

| Робота | Документація та результат |
|---|---|
| ЛР1 | [PDF](docs/lab_1/report.pdf), [дві збірки](docs/verification/lab1/reproducibility.json) |
| ЛР2 | [Pinout](docs/pinout.md), [архітектура](docs/architecture.md), [аналіз записів](docs/captures/analysis.md), [звіт](docs/lab2-report.md), [PDF](docs/lab_2/report.pdf) |

Перевірено Docker build, portable host-тести та реальні запуски Wokwi. CI є конфігурацією GitHub Actions; віддалений запуск не заявляється. Фізичну плату й інтерактивну Ubuntu VM не перевірено. ЛР1 запускається з тегу `lab1` після чистої збірки; після повернення до `main` потрібна нова чиста збірка.
