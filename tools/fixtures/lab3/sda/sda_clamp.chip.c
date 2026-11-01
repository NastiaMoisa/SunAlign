#include "wokwi-api.h"
static pin_t sda, marker;
static int phase;
static void step(void *unused) {
    (void)unused;
    if (phase++ == 0) {
        pin_mode(sda, OUTPUT_LOW);
        pin_write(marker, HIGH);
    } else {
        pin_mode(sda, INPUT);
        pin_write(marker, LOW);
    }
}
void chip_init(void) {
    sda = pin_init("SDA", INPUT);
    marker = pin_init("CLAMP", OUTPUT_LOW);
    timer_config_t cfg = {.callback=step, .user_data=0};
    timer_t timer = timer_init(&cfg);
    timer_start(timer, 5000000, true);
}
