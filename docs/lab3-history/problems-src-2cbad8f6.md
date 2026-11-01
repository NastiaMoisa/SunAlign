# Підтверджені проблеми ЛР3

Виправлення є у робочому дереві. Нові коміти не створювалися.
Фінальні S1/S2/S3 використовують src-2cbad8f6d7e5.
Пілоти попередніх версій залишені як негативні результати.

| Симптом | Встановлена причина або межа знання | Дія | Первинний доказ |
|---|---|---|---|
| FLASH overflow 188 B після резервування сховища | printf і програма ділили 28 KiB | integer/string serial_text замість універсального форматування | build-first.log, build-integer-telemetry.log |
| undefined _isatty/_fstat при LTO | newlib потребує syscall symbols, оптимізація їх вилучала | syscalls.o без LTO | build-first.log, firmware/Makefile |
| .su немає для LTO .o | кадри виникають під час link-time generation | -fstack-usage і -save-temps=obj також при лінкуванні | stack-frames-final.su |
| BUILD_ID порожній, git не знайдено в Docker | Git не входить до мінімального image | hash компільованих джерел Python, compare-before-update header | build-observe-fix.log, tools/source_id.py |
| .ioc не описував нові маркери/TIM3 та 2048 B stack | ручні зміни не були внесені до опису конфігурації | актуалізація .ioc, перевірка pinout | ioc-before-alignment.txt, check_pinout.py |
| Часові delta близько 2^32−65536 us | попередня IRQ-схема давала локальний зворотний стрибок біля TIM3 wrap у моделі | sampled16 accumulator без overflow IRQ; перевірка VCD | pilot-tim3.log/vcd, pilot-sampled.log/vcd, S1 |
| ZERO timestamp був 0 за прийнятої кнопки | старий шлях EXTI не надав підтвердженої мітки події; точна причина моделі не встановлена | poll PB0 кожну 1 ms, явно src=poll; повна реакція через PB0/PA7 VCD | pilot-storage-after-zero.log, остаточний ZERO log |
| SAVE ok=0 після справжнього ZERO | причина Flash ще не розрізнена: рання HAL помилка або невідповідний readback | цілу сторінку перевірено до програмування, невдача не видається за успіх; окрема діагностика потрібна | pilot-storage-after-zero.log |
| TX lost під час рідких c+d та t | shared TX 1023 B приймає burst статистики й snapshot без backpressure | лічильник втрат і статус з обмеженням; прошивку під час S2 не змінювали | S2.log, RES tx_lost |

Для Flash рядок ff_words=0 bad=0 не встановлює успіх HAL erase:
такі початкові значення залишаються і до входу в перевірку сторінки.
Тривалість невдалої спроби не є виміром фізичного стирання.
Остаточне пояснення доповнюється лише після перевірки поверненого HAL status.

Часові помилки пілотів є проблемами вимірювального інструментарію, а не
доказом порушення всіх продуктових дедлайнів. Дані цих версій не змішують
із таблицею фінальних сценаріїв. Регенерацію CubeMX не виконували.

## Остаточний аудит

Незалежний VCD підтвердив розбіжність TIM3 у S2/S3; причина не встановлена. CPU maxima та deadline counters цих сценаріїв не вважаються доведеними. Старий host PANEL prefix-parser помилково завершив S3-attempt1 через пошкоджений рядок. Його raw збережено, validation відхиляє завершення; full-record parser з діапазонами й step bound використано для остаточного повтору. NACK collector спершу пропустив проміжний ERROR у chunk; історію накопичено, повтор30 завершено. GDB нульові сторінки та unsupported Z0/Z1 не визначили HAL status; SAVE лишився ok=0.
