# SunAlign

ЛР4: [реалізація й результати](docs/lab4-report.md), [дизайн задач](docs/rtos-design.md), [порівняльна таблиця та гіпотези](docs/comparison.md). Збережено два LDR, MPU6050, ZERO та PANEL/POINT interface. FreeRTOS-Kernel V10.5.1, GCC/ARM_CM0, Nucleo-C031C6. Поточний `make build` збирає FreeRTOS; точні виміряні ELF і автономні source/recipes визначено у звіті. `wokwi.toml` є конфігурацією окремого demo-запуску й не перезаписує виміряні captures.

Основне парне порівняння14Nov: super loop S3 — 60 ZERO, max32,3375 мс, dispatch max2 мс, LED late2 мс, TX lost2489 B. RTOS не завершив повний S3, попри native tick до30,101 с. S2 бази15Nov незалежно підтверджено:3605498 мс до цілі колектора,3786411,846075 мс wire VCD. RTOS S2 failed після одного PANEL; повних годинних CPU/task metrics немає. Failed FR: Lab3 FR13; FreeRTOS FR02–04,06–14. Реальний Flash SAVE0/30 залишається невдалим, host recovery перевіряє окремо алгоритм storage.

Оригінальна виміряна ЛР3: `src-af2d5aa374d1`, ELF SHA-256 `9791f5375be0fea0e576c3d3c857ba3c16355593547e42675980834e502f8471`. Її результати відокремлено від інструментованої бази ЛР4 `33baeb35…96b3e` та FreeRTOS `3b87d18a…eb94`. Підсумки й метод — у [звіті ЛР4](../../../Lab04/Reports/ІО-36_Анастасія_Мойса_Лабораторна_робота_4.pdf).

Підказка орієнтації панелі.

Мойса Анастасія Святославівна, ІО-36.

Компоненти: MPU6050 I2C, два LDR ADC, кнопка GPIO.

Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Платформа Nucleo-C031C6.

ЛР1: відтворювана Docker-збірка, UART/LED, PRD і дослідження компонентів. Стан збережено тегом `lab1`.

ЛР2: MPU6050, два LDR та ZERO працюють разом. LED дає heartbeat, UART показує LEFT/RIGHT, LOWER/RAISE, DARK або ERROR. Нуль задається кнопкою й живе до reset. LDR не є повіреним люксметром.

`make build` збирає прошивку. Схема `diagram.json`, налаштування Wokwi `wokwi.toml`.

Документація: [PRD](docs/PRD.md), [компоненти](docs/hardware-components.md).

`make test` запускає host-тести й перевірку залежностей, `make coverage` вимірює покриття. У Serial Monitor команда `c` повторює ідентифікацію, `n` перевіряє відсутню адресу 0x69.

| Робота | Документація та результат |
|---|---|
| ЛР1 | [PDF](docs/lab_1/report.pdf), [дві збірки](docs/verification/lab1/reproducibility.json) |
| ЛР2 | [Pinout](docs/pinout.md), [архітектура](docs/architecture.md), [аналіз записів](docs/captures/analysis.md), [звіт](docs/lab2-report.md), [PDF](docs/lab_2/report.pdf) |

Перевірено Docker build, portable host-тести та реальні запуски Wokwi. CI є конфігурацією GitHub Actions; віддалений запуск не заявляється. ЛР1 запускається з тегу `lab1` після чистої збірки; після повернення до `main` потрібна нова чиста збірка.

ЛР3: [результати](docs/lab3-current.md), [задачі](docs/tasks.md), [формат метрик](docs/metrics/lab3/README.md), [обмеження](docs/limitations.md), [повторювані сценарії](docs/baseline-lab4.md). Додано `s` для спроби збереження ZERO, `d` для snapshot до 32 точок, `t` для статистики та діагностичний `r` для software reset. У [результатах](docs/lab3-current.md) наведено вимірювання та їхні межі: стійке збереження ZERO не підтверджено, під навантаженням зафіксовано втрати UART TX.


## Звіти та виміряна прошивка

- [ЛР3: фінальний звіт](docs/lab_3/report.pdf)
- [ЛР4: фінальний звіт](docs/lab_4/report.pdf)

Збережені виміряні ELF/HEX — `docs/firmware/lab4`; звичайний `wokwi.toml` використовує ці файли. Нова збірка не успадковує попередні вимірювання. Фактичні невиконані вимоги й межі наведено у звіті.
Великі історичні VCD збережено як `.vcd.gz`; перед відкриттям у логічному аналізаторі їх потрібно розпакувати.
