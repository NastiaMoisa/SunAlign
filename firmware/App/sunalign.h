#ifndef SUNALIGN_APP_H
#define SUNALIGN_APP_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool imu_ok, adc_ok, pressed;
    int32_t mg[3];
    uint16_t left, right;
    uint32_t imu_at, adc_at, sequence;
} panel_input_t;
typedef struct {
    bool candidate, debounced, zero_set, filtered, healthy, dark;
    uint32_t button_at, last_sequence, calibrations;
    float left, right, pitch, zero;
    int horizontal, vertical;
} sunalign_t;
void sunalign_init(sunalign_t *);
void sunalign_tick(sunalign_t *, const panel_input_t *, uint32_t);
const char *sunalign_horizontal(const sunalign_t *);
const char *sunalign_vertical(const sunalign_t *);
#endif
