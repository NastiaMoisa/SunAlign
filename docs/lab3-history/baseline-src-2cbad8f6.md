# Базова лінія SunAlign для ЛР4

ЛР4 тут не реалізована. Повторювати потрібно той самий стенд C031C6,
MPU6050 0x68, два LDR, PB0 ZERO, 48 MHz, UART115200 8N1 та всі вимірювальні
маркери. Порівнювати фінальні max, n, jitter, втрати й запаси пам'яті.
src-2cbad8f6d7e5 та ELF SHA256 зафіксовані у S1/S2/S3 manifests.
Код/буфери/логування мають бути однаковими між повторюваними сценаріями.

## Відтворення

make build, make test, make coverage. Зберегти ELF/HEX, map, size -A, nm
і production LTO .su. Не перескладати ELF/HEX під час активного Wokwi:
розширення може перезапустити симулятор. Кожен сценарій починається новим
запуском; run.vcd перейменовується тільки після зупинки.

Для Python потрібен pyserial. Локальний RFC2217 порт43614.
Нові імена raw обов'язкові; runner не перезаписує evidence.

```sh
python3 tools/run_lab3.py S1 --seconds 300 --out docs/metrics/lab3/repeat-S1.log
python3 tools/run_lab3.py S2 --seconds 4320 --out docs/metrics/lab3/repeat-S2.log
python3 tools/run_lab3.py S3 --seconds 300 --rate 20 --out docs/metrics/lab3/repeat-S3.log
python3 tools/run_lab3_checks.py faults --count 30 --out docs/metrics/lab3/repeat-nack.log
python3 tools/run_lab3_checks.py startups --count 30 --out docs/metrics/lab3/repeat-startup.log
python3 tools/run_lab3_checks.py boundary --phase-seconds 10 --out docs/metrics/lab3/repeat-boundary.log
```

S1: LDR400/400 lux, X0/Y0/Z1 g, t раз на30 host s.
S2: c+d раз на60 host s, t раз на30 host s; відтворити світло/наклон/ZERO
за окремою таблицею перших спостережень контрольних станів.
S3: s+d потік20 host bursts/s, кожний burst2bytes; отримати >=30 фізичних
подій PB0 під час PA6HIGH, змінюючи фазу натискання. Фактичний S3 почав потік ще до першого ZERO; для живої демонстрації задати ZERO перед s.
Довга операція є справжнім UART snapshot export до TX idle.
Частий d перезапускає export; не припускати завершення кожного запиту.

NACK test адресує відсутню0x69 і чекає ERROR/recovery кожного циклу.
Це не від'єднання сенсора. Startup r є software reset; FIRST відраховано
від observe_init, не від power-on. Реальні SDA fault, power ramp і power-cut
потребують фізичного стенда або окремо перевіреної fault fixture.

## Незалежний вимір

D0SCL, D1SDA, D2UARTTX, D3ADCpair, D4PB0, D5LED, D6PA6export/Flash,
D7PA7acceptedZERO. База VCD1ns, buffer24M transitions.
analyze_lab3.py читає raw; analyze_lab3_vcd.py вибирає перші n ADC інтервалів
за останнім STAT adc_dt, щоб зіставити однакові спостереження.
decode_lab3_uart.py дає wire timestamps і framing errors.
ZERO VCD починається першим falling після>=30msHIGH; наступні bounce
falls зберігаються окремо. UART sample timestamp має коротший зміст.

TIM3sampled16 потребує читань<65.536ms; одного gap_mod недостатньо.
У S2/S3 VCD підтвердив дефект TIM3. Перед порівнянням RTOS
потрібно виправити і повторно перевірити шкалу, а не порівнювати
її недостовірні maxima як CPU latency. Повний CPU profiler і фізичний Flash час не отримані.

## Зафіксовані результати

| Метрика | S1 | S2 | S3 |
|---|---|---|---|
| loop | 251/783/2463; n=383733 | 251/1038/67042; n=4162075 | 251/1659/132896; n=175981 |
| command | 62/362/905; n=11 | 41/1619/65837; n=294 | 39/2803/132839; n=10640 |
| button | немає події | 29820/30371/30881; n=6 | 29703/30880/32575; n=51 |
| adc_dt | 96925/99999/103093; n=3005 | 74998/100000/126778; n=43205 | 31569/100033/216016; n=3015 |
| imu_dt | 98150/100000/101753; n=3004 | 35431/100156/269025; n=43136 | 33389/100044/210719; n=2310 |
| panel_dt | 711/1002/2428; n=299778 | 711/1095/66970; n=3945207 | 518/1668/132788; n=180773 |

| Ресурс | S1 | S2 | S3 |
|---|---|---|---|
| stack_used | 624 | 640 | 680 |
| heap_peak | 0 | 0 | 0 |
| rx_lost | 0 | 0 | 0 |
| rx_errors | 0 | 0 | 0 |
| tx_lost | 0 | 644 | 1693761 |

Flash23496/28672B +4096B reserve; RAM6136/12288B з reserve.
main frame376B, відомий static chain456B, повна верхня межа не доведена.
Оцінка service_permille не є повним CPU usage.

Після виправлення шкали гіпотеза: panel max зменшити на20% без зміни1msrelease; tx_lost=0 у S2
без збільшенняTX; пауза c не зникне від самоїRTOS; RAM/Flash зростуть;
прийняті export мають завершуватися за явною ACCEPT/BUSY політикою;
CRC алгоритм залишиться тим самим. Конкретні механізми, ціна й обмеження
в limitations.md. Результат гіпотези встановлює майбутній повтор, не цей текст.

## Незалежні підсумки остаточних записів

S1/S2/S3 completed=true на одному ELF. Повні VCD мають приблизно
345 s /4331.610 s /323.334 s. Це інші межі, ніж host-вікна 300.498 /
4320.498 /301.499 повідомлених MCU s. S2 справді перевищив годину.

| Незалежний сигнал | n | min/avg/max, us |
|---|---|---|
| S1 ADC |3005|96925.025/99999.740/103092.350|
| S2 ADC |43205|35670.475/99995.445/103924.400|
| S3 ADC |3015|31584.225/98352.596/104679.300|
| S3 LED |603|2826.075/490771.287/502473.375|
| S3 ZERO усі |51|30928.275/33147.209/36288.575|
| S3 ZERO під час PA6 HIGH |50|30928.275/33141.016/36288.575|

ZERO до 50 ms підтверджено для цих записаних модельних подій.
Теорія UART8N1: 10N/baud. S2 PA6 від55859782625ns містить1357 frames
усіх потоків. Нижня межа при115200baud=117795.139us, при BRR417/48MHz=
117889.375us; VCD HIGH=142476.875us, на20.86% більше другої межі.
HIGH включає dispatch і проміжки рядків, кожний не частіше1ms, та
конкуруючу телеметрію. Це не чиста передача POINT і не TIM3 CPU timing.
Прикордонний frame може змінити N на1. Дані в S2-wire.json.
UART ZERO рядки втрачені частково, тому неповні послідовності не спарюються
з GPIO за порядком. Clock JSON містить відкинуті PANEL та residual windows.
У S3 framing errors=0, 21 malformed/backward PANEL виключено.

30/30 адресних NACK/recovery, 30/30 software reset і 30/30 нових Start/Stop
моделі завершено. Fresh t=0 ADC GPIO=6220.2 us; перша готовність IMU UART=
110760.8 us у всіх 30 модельних стартах. Це детерміновані модельні значення.
UART є верхньою оцінкою повідомлення, не чистим часом завершення сенсора.
Software reset і модельний Start не замінюють фізичне ввімкнення живлення.

UART boundary має 8 фаз по 10 MCU s, запити 1/5/10/20/50/100/500/1000 d/s.
Перша доведена втрата TX при запиті 20 d/s, RX drops/errors=0 у всіх фазах.
Внутрішній deadline при 10 d/s через timer не є доведеним фізичним порогом.
Fault fixture того самого ELF показав ERROR/recovery після AD0 і SDA clamp.
У цій схемі D5=AD0, не LED. Базову схему після досліду відновлено.

SAVE ok=0 у спостережених спробах. Дві прочитані перед дослідом Flash
сторінки були нульовими; GDB Z0/Z1 не підтримав breakpoint. Статус HAL
після erase невідомий. Це не доказ HAL_OK чи загальної відсутності Flash
моделі. Успішне збереження/readback/reset/power-cut не підтверджено.

S3-attempt1 окремо відхилено: старий host parser прийняв пошкоджений PANEL,
хибно completed=true; full-record replay знайшов близько 214.5 s.
Після виправлення parser остаточний S3 повторено. NACK-attempt1 містить
timeout collector, а не доведений hardware hang; 30-цикл повтор завершено.

Stack S3 peak=680 B залишив спостережені 1368 B із резерву 2048.
Це не повний static bound. Чиста повторна збірка дала ідентичні ELF/HEX/BIN.
Поточні hashes у build-manifest.json і reproducibility.json.
