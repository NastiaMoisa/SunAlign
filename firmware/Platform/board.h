#ifndef SUNALIGN_BOARD_H
#define SUNALIGN_BOARD_H
#include "sunalign.h"
#include "mpu6050.h"
void board_init(void);
bool board_tick(uint32_t);
extern uint32_t board_adc_sequence, board_adc_us;
sensor_bus_t board_bus(void);
panel_input_t board_inputs(const mpu6050_t *);
char board_command(void);
bool board_tx_idle(void);
extern volatile uint32_t board_rx_lost, board_tx_lost, board_rx_errors;
extern uint32_t board_command_at;
#endif
