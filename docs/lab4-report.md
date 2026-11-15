# ЛР4 SunAlign

Реалізовано задачі panel/sensors/io та повторно використано App/Devices. Ядро FreeRTOS-Kernel V10.5.1, MIT, GCC/ARM_CM0, heap_4; MCU Nucleo-C031C6. [RTOS design](rtos-design.md) пояснює пріоритети, власників шин, черги й ISR notifications. [Порівняння](comparison.md) містить результати та всі п'ять гіпотез ЛР3.

Виміряні ELF SHA-256:

- Super loop із GPIO: `33baeb35702c35c7f47abb04735c873c52b2c0aa72ff855fed34053c88d96b3e`.
- FreeRTOS product: `3b87d18a244b7819b61c68df72bc1b69cca62138374a0e9f08aac67a22ceeb94`.

У S3 суперцикл прийняв60 ZERO під експортом, max32,3375 мс; TX lost2489 B. RTOS S3 не завершено, хоча native tick продовжувався близько30 с. Діагностичні OS1–5 перевірили mutex/binary, stack hook on/off, starvation, queue timeout/drop і heap failure; зміна частоти tick1000/100Hz дала лише раннє порівняльне вікно перед відмовою повного S3. Невдалі інтеграційні FR і SAVE0/30 збережено як фактичні результати.

S2 бази15Nov підтверджено UART та незалежним VCD:3605498 мс до цілі, один BOOT, monotonic, framing0. RTOS S2 failed після одного PANEL. Повного парного годинного CPU/runtime-порівняння немає.

Фінальний PDF міститься в [матеріалах ЛР4](lab_4/report.pdf). Для відтворюваності автономні source archives, ELF/HEX/map, точні входи й recipes містяться у Lab04/Work/freeze-inputs-20261114; точний склад визначає freeze-20261114.json. Ці джерела збираються окремо й не підміняються поточним робочим деревом.
