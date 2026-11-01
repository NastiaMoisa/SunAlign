"""Regenerate technical Markdown tables from saved, completed raw-run analyses."""
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
folder = root / "docs/metrics/lab3"
FINAL_RESULTS = """
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
"""
scenarios = ["S1", "S2", "S3"]
data, manifests, captures = {}, {}, {}
for scenario in scenarios:
    for suffix, target in [(".json", data), (".log.json", manifests), ("-vcd.json", captures)]:
        path = folder / (scenario + suffix)
        if path.exists():
            target[scenario] = json.loads(path.read_text())

def stats(scenario, name):
    s = data.get(scenario, {}).get("statistics", {}).get(name)
    if not s or not s["n"]:
        return "немає події"
    return f'{s["min_us"]}/{s["avg_us"]}/{s["max_us"]}; n={s["n"]}'
def peak(scenario, field):
    values = [r[field] for r in data.get(scenario, {}).get("resources", [])]
    return max(values) if values else "немає даних"
def duration(scenario):
    m = manifests.get(scenario)
    if not m or not m.get("completed"):
        return "триває або не виконано"
    return str(((m["final_tick_ms"] - m["first_tick_ms"]) & 0xffffffff) / 1000)
def table(headers, rows):
    return "\n".join(["| " + " | ".join(map(str, headers)) + " |",
                       "|" + "|".join("---" for _ in headers) + "|"] +
                      ["| " + " | ".join(map(str, row)) + " |" for row in rows]) + "\n"

text = """# SunAlign, ЛР3: технічні результати

Мойса Анастасія Святославівна, ІО-36. Форма B, Wokwi.
Це стан фактично збережених дослідів. Повна готовність не заявляється:
успішне постійне збереження ZERO та physical power-cut не підтверджені.
Часові значення моделі не є гарантіями фізичного STM32.

## Реалізація й версія

Один кооперативний цикл без RTOS поєднує ADC, MPU6050, два LDR, ZERO,
LED, bounded UART, історію та snapshot 32 точок. Частина довгої UART-передачі
виконується за один короткий крок, приймання й передавання йдуть через IRQ.
Команди c/n повторюють ID/адресний NACK, s намагається зберегти ZERO,
d починає snapshot, t виводить 19 рядків, r робить software reset.

Остаточна збірка src-2cbad8f6d7e5, ELF SHA256
5a3df301d02ea13713421ccff9a77587f0677cb3141a6780f92e85e87f18ac3e.
S1/S2/S3 використовують цей самий ELF. Пілоти інших src-ID збережені окремо.
Плата Nucleo-C031C6, Cortex-M0+, HSI/AHB/APB 48 MHz, UART 115200 8N1.
HAL 1.0.1, CMSIS Core 5.3.0, ARM GCC 13.2.1.
[Задачі](tasks.md), [архітектура](architecture.md), [схема](lab_3/img/superloop.png).

SunAlign порівнює B=4095-ADC, IIR alpha=1/4, пороги світла 15/8%,
нахилу 5/3°, DARK при сумі B<500. Придатний вектор 0.8..1.2 g.
ZERO має debounce 30 ms і задає відносний pitch.
Азимут Сонця та енергетичний виграш не визначаються.

## Метод і межі достовірності

TIM3 1 MHz, PSC=47, ARR=65535. Короткі захищені читання накопичують
uint16 delta у 64-bit сумі; між читаннями має минути менше 65.536 ms.
TIM3 overflow IRQ та EXTI0_1 вимкнені. PB0 опитується кожну 1 ms.
UART ZERO дає sample-to-accept, а VCD PB0-to-PA7 дає повну реакцію
від першого falling після 30 ms HIGH, включно з наступним брязкотом.
PA6 позначає union активного export до TX idle та Flash call.
Цей пін не ізолює Flash, якщо операції перекриваються.

Пілот IRQ-таймера мав хибні delta біля wrap і відхилений.
sampled16 пройшов коротку незалежну перевірку та S1.
У S2 та S3 незалежний VCD підтвердив недостовірність TIM3 шкали.
S2 має три residual steps близько -65.5 ms у 30 s вікнах 2010/2400/2790 s.
ADC S2 VCD max=103924.4 us проти TIM3 126778 us при однакових n=43205.
S3 має багато аналогічних розбіжностей. Причина не встановлена.
Ці внутрішні extrema не доводять CPU latency або deadline.
gap_mod_us не бачить цілих пропущених обертів.

У S1 однакові 3005 ADC інтервалів дали TIM3
96925/99999/103093 us і VCD 96925.025/99999.740/103092.350 us.
Екстремуми відрізняються менше ніж на 1 us; UART avg цілочисельний.
UART D2 декодовано без framing errors. Це підтверджує метрику моделі,
а не точність фізичного ADC або живлення.

## Сценарії

S1: 400/400 lux, X0/Y0/Z1 g, тільки періодичні задачі та t раз на 30 host s.
S2: c+d раз на 60 host s, t раз на 30 host s, реальні GUI зміни світла,
наклону й ZERO. Ціль 4320 MCU s для години й 32-bit us rollover.
S3: потік s+d, періодичні задачі та натискання ZERO під час активного export.
Host rate означає запитані bursts, S3 burst має дві команди.
S3 фактичні GUI входи: LDR100000/100000 lux, X=-0.55,Y=0,Z=0.9 g.
Видима частота з лічильників обмежена недостовірною MCU шкалою.

"""
text += table(["Сценарій", "Завершена тривалість MCU, s", "Stack peak, B", "Heap peak, B", "TX lost, B"],
              [[s, duration(s), peak(s,"stack_used"), peak(s,"heap_peak"), peak(s,"tx_lost")] for s in scenarios])
text += "\nУ таблиці повідомлені TIM3 min/avg/max; n є кількістю спостережень. S2/S3 значення недостовірні для CPU висновків. Через TX loss останні повні STAT різних метрик можуть походити з різних звітів.\n\n"
text += table(["Метрика", "S1", "S2", "S3"],
              [[name] + [stats(s,name) for s in scenarios] for name in
               ["board","imu","panel","led","log","loop","command","button","save","dump_wire",
                "adc_dt","imu_dt","panel_dt","led_dt","log_dt"]])
text += """
save містить усі спроби, включно з невдалими; успіх перевіряється окремим ok.
У S1 panel пропустив 760 release і одне завершення за наступною ms-межею.
service_permille є часткою корисного обслуговування останньої секунди,
без повного обліку ISR. Цикл не спить. Це не повне завантаження чи
енергоспоживання CPU. У S1 останнє значення 444, тобто 44.4%.

## Пам'ять

"""
text += table(["Показник", "Значення"], [
    ["Flash code", "23496/28672 B, залишок 5176 B"],
    ["Flash reserve", "4096 B, дві сторінки ZERO"],
    ["RAM із резервами", "6136/12288 B, залишок 6152 B"],
    [".data / .bss", "152 / 3420 B"],
    ["Мінімальні резерви stack / heap", "2048 / 512 B"],
    ["Найбільший власний frame", "main 376 B"],
    ["Підтверджений static chain", "main→atan2f→ieee754_atan2f→atanf→fadd =456 B"],
    ["TX / history / snapshot / stats", "1024 /384 /384 /360 B"],
    ["RX bytes + timestamps", "64+256 B, 63 корисні entries"],
])
text += """
Static chain є перевіреним шляхом, а не повною верхньою межею; callbacks,
exception frames та вкладені IRQ потребують повного call-graph аудиту.
Painting обмежене 2048 B MSP і дає watermark виконаних шляхів.
Heap peak відстежує sbrk; прикладний код не викликає malloc.
512 B є мінімальним linker reserve, не окремим доведеним cap allocator.
LTO включив App/Devices/HAL у main, тому linked-symbol таблиця не є точним
розподілом за всіма початковими C-модулями.
[size](metrics/lab3/memory-final.txt), [символи](metrics/lab3/symbols-final.txt),
[LTO frames](metrics/lab3/stack-frames-final.su), [static path](metrics/lab3/stack-static-final.json).

## Перевірки й статус FR

Host: 8 успадкованих груп та CRC/fallback/sequence wrap, 24 torn-prefix,
192 bit flips, cadence skip/wrap і serial INT_MIN. ASan/UBSan проходять.
App/Devices coverage 125/126 рядків, 140/145 гілок.
HAL/IRQ/Flash не входять до portable coverage. Host prefix не є power cut.

"""
zero = captures.get("S3",{}).get("button_during_long")
zero_verdict = (f'Wokwi: n={zero["n"]}, max={zero["max_us"]:.3f} us; ' +
                ("ціль 50 ms виконана в записаних подіях" if zero["n"]>=30 and zero["max_us"]<=50000 else "ціль/повторюваність не підтверджена")) if zero else "S3 VCD ще не отримано"
text += table(["FR", "Статус і доказ"], [
    ["01", "Виконана в моделі: BOOT з версією, STORE/FIRST; 30/30 нових стартів із VCD"],
    ["02", "З обмеженням: S3 VCD max=502473.375 us, але ранні короткі зайві перемикання порушують регулярність"],
    ["03", "З обмеженням: ADC працює, але рівно 100 ms не гарантується, є jitter"],
    ["04", "З обмеженням: MPU працює, c повторює 100 ms settle; точні внутрішні S2 extrema недостовірні"],
    ["05", "Виконана у portable тестах і модельних переходах, alpha=1/4"],
    ["06", "Виконана функціонально в S2 LEFT/CENTER/RIGHT та host гістерезисі"],
    ["07", zero_verdict],
    ["08", "Виконана функціонально: UNSET, ZERO, LOWER/LEVEL/RAISE у S2"],
    ["09", "Виконана функціонально: DARK при ADC 4063/4063 у S2, host пороги"],
    ["10", "З обмеженням: portable errors/stale/invalid g; адресний fault окремо від фізичного"],
    ["11", "З обмеженням: retry і відновлення AD0/SDA в окремій модельній схемі; фізичні SDA/power не перевірені"],
    ["12", "c/n інтегровані; кількість нових NACK/recovery підтверджує окремий raw trial"],
    ["13", "Не виконана в доступному MCU-пілоті: SAVE ok=0; host CRC не замінює controller/power-cut"],
    ["14", "З обмеженням: bounded snapshot/метрики, але TX втрачає байти в S2; boundary окремо"],
])
text += """

Flash adapter викликає справжні HAL erase/program, перевіряє FF до
програмування й робить volatile readback. ff_words=0 bad=0 неоднозначний
без HAL status, оскільки це також початкові значення до перевірки.
Невдала спроба не є успішним erase або виміром його фізичного часу.
Перевірка цілісності після реального зникнення живлення не виконана.

[Реєстр обмежень](limitations.md) містить симптоми, докази, bare-metal обходи,
ціну RTOS та числові гіпотези. [Журнал проблем](problems.md) відділяє
доведені причини від нерозв'язаних. [Базова лінія](baseline-lab4.md) задає
повторення. S2/S3 max і незалежна перевірка годинника мають пріоритет над
загальними твердженнями про придатність RTOS. ЛР4 тут не реалізовано.
"""
(root/"docs/lab3-report.md").write_text(text + FINAL_RESULTS)

baseline = """# Базова лінія SunAlign для ЛР4

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

"""
baseline += table(["Метрика", "S1", "S2", "S3"],
                  [[name]+[stats(s,name) for s in scenarios] for name in ["loop","command","button","adc_dt","imu_dt","panel_dt"]])
baseline += "\n" + table(["Ресурс", "S1", "S2", "S3"], [[field]+[peak(s,field) for s in scenarios] for field in ["stack_used","heap_peak","rx_lost","rx_errors","tx_lost"]])
baseline += """
Flash23496/28672B +4096B reserve; RAM6136/12288B з reserve.
main frame376B, відомий static chain456B, повна верхня межа не доведена.
Оцінка service_permille не є повним CPU usage.

Після виправлення шкали гіпотеза: panel max зменшити на20% без зміни1msrelease; tx_lost=0 у S2
без збільшенняTX; пауза c не зникне від самоїRTOS; RAM/Flash зростуть;
прийняті export мають завершуватися за явною ACCEPT/BUSY політикою;
CRC алгоритм залишиться тим самим. Конкретні механізми, ціна й обмеження
в limitations.md. Результат гіпотези встановлює майбутній повтор, не цей текст.
"""
(root/"docs/baseline-lab4.md").write_text(baseline + FINAL_RESULTS)
print("Generated technical Markdown from:", list(data), "completed:", [s for s,m in manifests.items() if m.get("completed")])
