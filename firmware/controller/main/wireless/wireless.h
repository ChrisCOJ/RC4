#pragma once

#include <stdint.h>


#define MAX_STEERING_ANGLE  30


typedef enum {
    CONTROLLER_STATE_OK = 0x00,
    CONTROLLER_THROTTLE_OFFLINE = 0x01,
    CONTROLLER_STEERING_OFFLINE = 0x02,
} controller_state_fail_codes;


typedef struct {
    // Essential Control
    uint8_t throttle_level;  // 0% - 100%  // Capped lower for instantaneous throttle (i.e. 60%)
    int8_t steering_angle;  // 0 = Ahead; MAX_STEERING_ANGLE = Left; -MAX_STEERING_ANGLE = Right.

    // MISC
    uint8_t drive_mode;  // 0 = Normal; 1 = Drift

    // Errors
    controller_state_fail_codes fail_code;  // 0 = OK; Other = Fault
} rc4_controller_state;



rc4_controller_state rc4_controller_get_state();