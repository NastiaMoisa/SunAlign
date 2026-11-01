# Окремий профіль foreground-задач SunAlign

Один короткий запуск Wokwi на діагностичній збірці `src-b16af10ff4b0`, ELF SHA-256 `3bc51b829e7e8e3804919fe8866ed4408f314d19144638cf91c4123de81f19aa`, HEX `2a300391510cc4c94905c96271607d3fffd39c7ef373cb390ac5123480c01f88`. Це інша прошивка, ніж `src-af2d5aa374d1`, використана для S1–S3 і boundary-drain. Отримане значення не є навантаженням годинного S2.

Схема і виміряні ELF/HEX містяться у [workspace](workspace/); [UART](results/uart.log), [статус і команди](results/status.json), [VCD.gz](results/cpu-profile.vcd.gz) і [розрахунок](results/profile.json) збережено окремо. У копії status єдина зміна — приватний шлях workspace замінено на відносний. Початковий UART SHA-256 `b0ced4466da9adfc813cc87fc0e421ea815816ecbdef2430a4389c4a496d59b0`; розпакований VCD SHA-256 `9d85d11d7b5e4b91d3209c3058164a067cd5dc12aceeb253d9736795980b8203`. Схема SHA-256 `02749b7b8dede780ab051b91390ea369ba886c4452ae12b76afcd2427ed75c1a`.

D6 позначає окремі виконувані ділянки foreground-задач, включно з активним опитуванням ADC і I2C, обробкою команд і формуванням експорту. За 11 повних секунд VCD min/avg/max частки HIGH становив 44,309/44,370/44,878%. Для 12308 інтервалів HIGH min/avg/max 3,5/396,545/753 мкс. Це суміш ділянок, можливе обрізання на межах вікна, не WCET задач. Порожні перевірки, spin і ISR поза HIGH не входять; ISR усередині HIGH входять. Вставлені маркери змінюють час. 40 порожніх імпульсів по 0,25 мкс показують ширину маркера, але не повні витрати інструментування. Асинхронний час передачі UART не є часом CPU; прив'язка до PANEL має затримку черги. VCD не встановлює фізичної точності 1 нс. Фізичну плату не виміряно.

Вікно починається від PANEL 2500 мс, обрізається до 11 повних секунд VCD перед опорною точкою PANEL 14500 мс. Команди, UTC та фактична тривалість записані у status. У режимі колектора `fault` для цього профілю `n` не надсилалася; виконано по одній `d` і `c`.

Для повторного аналізу збереженої траси з кореня репозиторію:

```sh
gzip -dc docs/metrics/lab3/cpu-profile-20261101/results/cpu-profile.vcd.gz > /tmp/sunalign-cpu.vcd
python3 tools/decode_lab3_uart.py /tmp/sunalign-cpu.vcd --output /tmp/sunalign-cpu-wire.json
python3 docs/metrics/lab3/cpu-profile-20261101/tools/analyze_cpu_profile.py /tmp/sunalign-cpu.vcd /tmp/sunalign-cpu-wire.json --output /tmp/sunalign-cpu-profile.json
```

[Підготовка маркерів](tools/prepare_cpu_profile.py) застосовується до ізольованого `source/`, отриманого через `git archive 3511373543782ca26217374bd77c00cb3fb7a0bc`; не застосовуйте її до робочої прошивки. [Колектор](tools/collect_cpu_profile.py) і [аналізатор](tools/analyze_cpu_profile.py) збережені для перевірки методу. Новий запуск потребує окремої копії workspace та нових файлів; він не замінює цей запис.
