#pragma once

#include <stdint.h>


typedef struct {
    float battery_voltage;

    // error codes
    uint8_t steering_control_error;  // 1 = generic error, 0 = OK. Temporary codes
    uint8_t throttle_control_error;  // 1 = generic error, 0 = OK. Temporary codes
}rc4_chasssis_telemetry;