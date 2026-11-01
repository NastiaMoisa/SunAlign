# SunAlign

Актуальний вихідний код має ідентифікатор `src-af2d5aa374d1`. У Wokwi на цій збірці підтверджено безперервний годинний S2 та окремий навантажений S3. Три спроби збереження повернули помилку; фізична плата, power-cut, CPU load та субмілісекундний WCET не перевірені. Повна готовність ЛР3 не заявляється.

Увага: наявний локальний `firmware/build/firmware.elf` може містити попередню збірку; перед запуском перевіряйте BOOT та SHA-256. Заморожений ELF досліду має SHA-256 `9791f5375be0fea0e576c3d3c857ba3c16355593547e42675980834e502f8471`.

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

ЛР3 у робочій гілці `feature/lab-3`: [результати](docs/lab3-current.md), [задачі](docs/tasks.md), [формат метрик](docs/metrics/lab3/README.md), [обмеження](docs/limitations.md), [повторювані сценарії](docs/baseline-lab4.md). Додано `s` для спроби збереження ZERO, `d` для snapshot до 32 точок, `t` для статистики та діагностичний `r` для software reset. Актуальний стан і доступ до доказів описано в [результатах](docs/lab3-current.md). Повна готовність ЛР3 не заявляється.
