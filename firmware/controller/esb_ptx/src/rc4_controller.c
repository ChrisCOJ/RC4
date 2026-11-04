#include <stdint.h>

#include "../include/rc4_controller.h"



rc4_controller_state rc4_controller_get_state(){
    rc4_controller_state state = {0};
    uint8_t err;

    uint8_t throttle = 0;
    err = get_throttle_value(&throttle);
    if (err) {
        state.fail_code |= CONTROLLER_THROTTLE_OFFLINE;
    }
    state.throttle_level = throttle;

    uint8_t steering_angle = 0;
    err = get_steering_angle(&steering_angle);
    if (err) {
        state.fail_code |= CONTROLLER_STEERING_OFFLINE;
    }
    state.steering_angle = steering_angle;

    state.drive_mode = get_drive_mode();

    return state;
}