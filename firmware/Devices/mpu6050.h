#ifndef SUNALIGN_MPU6050_H
#define SUNALIGN_MPU6050_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { BUS_OK, BUS_PENDING, BUS_NACK, BUS_TIMEOUT, BUS_ERROR, BUS_BUSY } bus_status_t;
typedef struct {
    void *context;
    bus_status_t (*start)(void *, bool, uint8_t, uint8_t, uint8_t *, uint16_t);
    bus_status_t (*poll)(void *);
} sensor_bus_t;
typedef enum { MPU_ID, MPU_WAKE, MPU_FILTER, MPU_RANGE, MPU_SETTLE,
               MPU_READY, MPU_SAMPLE, MPU_PROBE, MPU_RETRY } mpu_state_t;
typedef struct {
    sensor_bus_t bus;
    mpu_state_t state;
    bool waiting, online, valid, probe_requested, refresh_requested;
    uint8_t bytes[6], id;
    int32_t mg[3];
    uint32_t deadline, last_sample, sequence, errors;
    bus_status_t last_error;
} mpu6050_t;
void mpu6050_init(mpu6050_t *, sensor_bus_t, uint32_t);
void mpu6050_tick(mpu6050_t *, uint32_t);
void mpu6050_request(mpu6050_t *, bool);
void mpu6050_decode(const uint8_t[6], int32_t[3]);
#endif
