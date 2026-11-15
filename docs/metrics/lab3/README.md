# Вимірювання ЛР3

S1/S2/S3 completed=true на src-2cbad8f6d7e5. TIM3 S2/S3
недостовірний для CPU latency; незалежні висновки у *-clock.json,
*-vcd.json та ../../lab3-report.md.

| Рядок UART | Значення |
|---|---|
| STAT name n min avg max | Накопичено від старту, us; avg цілочисельний |
| ZERO sample_us accept_us n src=poll | Перший опитаний LOW і прийняття ZERO, не ISR timestamp |
| SAVE start_us duration_us ok | Спроба збереження; ok=0 не є успішним записом |
| ERASE page ff_words bad | FF-слова після успішного HAL; початкові нулі також можливі при ранній помилці |
| PANEL tick_ms L R pitch_cdeg horizontal vertical zero_count imu_errors | Час MCU, ADC та підказки |
| SCHED panel_late_ms panel_skips led_late_ms log_late_ms | Запізнення release та пропущені panel release |
| DEADLINE panel led log command | Завершення за deadline; panel із кроком ms |
| RES stack_bytes heap_peak_bytes rx_drops uart_errors tx_drops service_permille | Ресурси й останнє сервісне вікно |
| CLOCK mode=sampled16 reads wraps gap_mod_us | Не самодостатній доказ відсутності пропущених wraps |
| EXPORT_DONE start_us duration_us | Export до спорожнення TX |

service_permille оцінює корисне обслуговування без порожніх перевірок і без
повного обліку ISR. Цикл не спить. Це не повне завантаження або споживання CPU.
Нульова кількість STAT означає відсутність спостережень, у JSON min/avg/max
для цього випадку є null.

Runner використовує MCU PANEL для тривалості, host час лише для команд.
--rate є запитаною частотою host bursts. S3 burst містить s+d.
Видиму частоту з STAT рахувати окремо, з обмеженням недостовірної шкали.
Скрипт відмовляється перезаписувати raw logs. Manifest містить SHA256
ELF, схеми, журналу, source ID, тривалість та completed.

RFC2217 може повернути попередній serial prefix. Raw лишається незміненим.
Аналізатор вибирає останній BOOT, runner вимагає source ID і перший PANEL
не пізніше 1000 ms для початку сценарію. При tx_lost рядки можуть бути неповними.

Окрема diagram.events.json є незапущеним резервним stimulus fixture.
Вона не є первинним доказом виконання S1/S2/S3.
