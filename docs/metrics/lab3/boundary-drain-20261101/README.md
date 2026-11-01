# Короткий boundary-drain SunAlign

Виміряна прошивка `src-af2d5aa374d1`, ELF SHA-256 `9791f5375be0fea0e576c3d3c857ba3c16355593547e42675980834e502f8471`; [схема та фактичні ELF/HEX](workspace/) збережені як окремий стенд. SHA-256 схеми `02749b7b8dede780ab051b91390ea369ba886c4452ae12b76afcd2427ed75c1a`, HEX `9e23b94f19d66e8e53f2d3d419ffa3e8b3c0fb5c9e017ccd0d625c08bed4bad8`.

[Умови і результати](../current/boundary-drain-evidence.md); [UART](results/uart.log), [фази і команди](results/status.json), [VCD.gz](results/capture.vcd.gz), [колектор](collect_boundary_drain.py). UART SHA-256 `0d374c74aa2daad49d0c21657366e5f229a6502bad33475dcd5a8eea6ee5ca20`, розпакований VCD SHA-256 `d41003889a5e1a55b41987265ed2bbab5a6e5e369189215875a49223184daced`.

За фактичних частот відправлення host 1,967/4,719/9,493/16,004/24,054 запиту/с приріст TX lost після відповідних етапів і пауз становив 0/0/0/24003/74847 байтів. Між 9,493 та 16,004 точний поріг невідомий; відправлені команди не тотожні завершеним експортам, а VCD не записує RX. Дані Wokwi не є вимірами фізичної плати.

Для незалежного декодування наявної траси з кореня репозиторію:

```sh
gzip -dc docs/metrics/lab3/boundary-drain-20261101/results/capture.vcd.gz > /tmp/sunalign-boundary.vcd
python3 tools/decode_lab3_uart.py /tmp/sunalign-boundary.vcd --output /tmp/sunalign-boundary-wire.json
```

Новий запуск потребує окремої копії workspace та нових файлів і не змінює цей запис.
