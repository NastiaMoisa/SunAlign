# Докази ЛР4 SunAlign

Первинні вимірювання зберігаються у власному `SP/Lab04/Work`. Імена нижче визначають точні набори, а не нові запуски.

- `runs/laneC-lab3-S1-20261114`, `runs/laneC-lab3-S3-fixturefix1-20261114` та `runs/laneC-rtos-{S1,S3}-20261114`: основне парне порівняння; status, mcu, vcd, wire та active ticks.
- `runs/laneC3-lab3-S2-wire-20261115`: verified S2.result.json, сирий uart.log, MCU/wire decode.
- `runs/laneC3-rtos-S2-20261115` і `runs/laneC2-rtos-S2-20261114`: failed S2.result.json.
- `os-suite-results.json`, `cost-pulses.json`, `os-tick-common-window-20261114.json`: діагностики ОС з окремими identities.
- `headless/host`: real-driver fakebus/storage/threshold tests та recipes; `headless/analysis/results.json`: метрики пам'яті й зведення.
- `freeze-20261114.json`, `freeze-addendum-20261115.json`: source/ELF/HEX/config/hash і межі застосування.

Host, ARM build і Wokwi — окремі рівні доказу. Відсутню метрику або failed-сценарій не замінено нулем. Докладна методика та числа — у [comparison](../../comparison.md).
