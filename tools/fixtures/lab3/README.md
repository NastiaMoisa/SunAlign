# Модельні fault fixtures ЛР3

Два окремі стенди використовують один і той самий ELF `src-af2d5aa374d1`, SHA-256 `9791f5375be0fea0e576c3d3c857ba3c16355593547e42675980834e502f8471`. Підсумок і точні hashes — `docs/metrics/lab3/current/model-faults-evidence.md`, сирі VCD — `docs/captures/lab3/current/`, UART і status — `docs/metrics/lab3/current/`. Збережені в репозиторії status не містять абсолютного шляху оператора; повні вихідні status збережено у локальному Lab03/Work.

- `sda/` залишає штатну модель MPU6050 і додає stimulus, який після 5 с симуляції утримує спільну SDA в нулі п'ять секунд, потім переходить у high impedance. D5 позначає утримання; D1 показує саму SDA. Це модельне, не фізичне замикання.
- `startup/` замінює MPU6050 мінімальним тестовим I2C-пристроєм: він постійно підключений до SCL/SDA за адресою 0x68, дає NACK до 3 с та ACK після. Модель реалізує лише регістри, потрібні SunAlign, і статичний вектор 1 g; це не модель фізичного запуску MPU6050. D5 показує READY. Відмова за адресою не перемикається на 0x69.

Для повторення спершу побудуйте firmware `make build`, перевірте SHA-256 ELF та скопіюйте один із цих каталогів під новою назвою поза деревом репозиторію. Скопіюйте до нього `firmware/build-current/firmware.elf` і `firmware/build-current/firmware.hex`; не редагуйте ці файли під час роботи Wokwi. Для нового запуску використовуйте новий каталог результатів:

```sh
python3 tools/collect_lab3_model_faults.py --workspace /path/to/copied-fixture --run-dir /path/to/new-run --mode fault
```

Collector потребує `pyserial` та підключається до локального RFC2217 43614; запустіть його до Start у Wokwi. Він не надсилає `n` або `c` у режимі fault. Після завершення штатно зупиніть тільки свою симуляцію, збережіть `capture.vcd`, `uart.log`, `status.json`, схему й hashes. Відсутність VCD, початкового здорового стану в SDA-тесті або потрібних фронтів не є успішним результатом. Для читання UART TX з VCD використовуйте `tools/decode_lab3_uart.py`. Для відтворення перевірки D5/D1/D3 і NACK/ACK у збережених VCD запустіть `python3 tools/verify_lab3_faults.py docs/captures/lab3/current/sda-runtime.vcd docs/captures/lab3/current/startup-delay.vcd`.

Host-моки моделей: із кореня репозиторію `node tools/fixtures/lab3/test_sda.js` та `node tools/fixtures/lab3/test_startup.js`. Вони перевіряють генератор, а не роботу MCU. Повноцінний I2C timeout, фізичне відключення/живлення та успішне збереження Flash ці досліди не підтверджують.
