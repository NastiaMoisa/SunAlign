#ifndef SUNALIGN_BOARD_H
#define SUNALIGN_BOARD_H
#include "sunalign.h"
#include "mpu6050.h"
void board_init(void);
void board_tick(uint32_t);
sensor_bus_t board_bus(void);
panel_input_t board_inputs(const mpu6050_t *);
char board_command(void);
#endif
