#include "analog_input.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>


// Shoulder triggers
#define SHOULDER_TRIGGER_ADC_MIN  1000
#define SHOULDER_TRIGGER_ADC_MAX  1800

// Joysticks
#define JOYSTICK_X_ADC_MIN     1200
#define JOYSTICK_X_ADC_MAX     2800
#define JOYSTICK_X_ADC_CENTER  1960
#define DEADZONE_LVL_1         50
#define DEADZONE_LVL_2         100
#define DEADZONE_LVL_3         150

#define MAX_STEERING_ANGLE   1000  // Normalized. Max right turn angle in degrees = 30 deg
#define MIN_STEERING_ANGLE  -1000  // Normalized. Max left turn angle in degrees = -30 deg

// Physical ADC channel IDs
#define LEFT_TRIGGER_ADC_ID     0
#define RIGHT_TRIGGER_ADC_ID    1
#define LEFT_JOYSTICK_X_ADC_ID  4

// Internal ADC array indices
enum {
    LEFT_TRIGGER_IDX,
    RIGHT_TRIGGER_IDX,
    LEFT_JOYSTICK_X_IDX,
    ADC_CHANNEL_COUNT
};


static controller_processed_analog_values current_inputs;
K_MUTEX_DEFINE(inputs_mutex);


static const struct adc_dt_spec adc_channels[ADC_CHANNEL_COUNT] = {
    [LEFT_TRIGGER_IDX] =
        ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), LEFT_TRIGGER_ADC_ID),

    [RIGHT_TRIGGER_IDX] =
        ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), RIGHT_TRIGGER_ADC_ID),

    [LEFT_JOYSTICK_X_IDX] =
        ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), LEFT_JOYSTICK_X_ADC_ID),
};


// Internal modules
static uint8_t adc_to_throttle_brake(int16_t raw)
{
    if (raw <= SHOULDER_TRIGGER_ADC_MIN) {
        return 100;
    }

    if (raw >= SHOULDER_TRIGGER_ADC_MAX) {
        return 0;
    }

    return (uint8_t)(
           ((int32_t)(SHOULDER_TRIGGER_ADC_MAX - raw) * 100) /
           (SHOULDER_TRIGGER_ADC_MAX - SHOULDER_TRIGGER_ADC_MIN));  // Normalized value between 0 to 100
}


static int adc_to_steering_angle(int16_t raw) {
    int deadzone_level = DEADZONE_LVL_1;

    if (raw <= JOYSTICK_X_ADC_MIN) {
        return MIN_STEERING_ANGLE;
    }

    if (raw >= JOYSTICK_X_ADC_MAX) {
        return MAX_STEERING_ANGLE;
    }

    if (raw <= (JOYSTICK_X_ADC_CENTER + deadzone_level) && raw >= (JOYSTICK_X_ADC_CENTER - deadzone_level)) {
        return 0;
    }

    int offset_center = raw <= JOYSTICK_X_ADC_CENTER
                 ? JOYSTICK_X_ADC_CENTER - deadzone_level
                 : JOYSTICK_X_ADC_CENTER + deadzone_level;
    int div = offset_center <= JOYSTICK_X_ADC_CENTER
              ? offset_center - JOYSTICK_X_ADC_MIN
              : JOYSTICK_X_ADC_MAX - offset_center;
    if (div == 0) return 0;
    int normalized = ((raw - offset_center) * 1000) / div;

    int cubic = (normalized * normalized * normalized) / 1000000;

    return (40 * normalized + 60 * cubic) / 100;
}


static int read_adc(uint8_t channel_index, int16_t *raw) {
    if (channel_index >= ADC_CHANNEL_COUNT || raw == NULL) {
        return -EINVAL;
    }

    struct adc_sequence sequence = {0};

    int err = adc_sequence_init_dt(
        &adc_channels[channel_index],
        &sequence
    );

    if (err < 0) {
        return err;
    }

    sequence.buffer = raw;
    sequence.buffer_size = sizeof(*raw);

    return adc_read_dt(&adc_channels[channel_index], &sequence);
}


// External modules
int analog_input_init(void) {
    int err;

    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        if (!adc_is_ready_dt(&adc_channels[i])) {
            return -ENODEV;
        }

        err = adc_channel_setup_dt(&adc_channels[i]);
        if (err < 0) {
            return err;
        }
    }

    return 0;
}


void analog_input_start(void* arg1, void* arg2, void* arg3) {
    int16_t raw_throttle;
    int16_t raw_brake;
    int16_t raw_steering;

    int throttle_err;
    int brake_err;
    int steering_err;

    while (1) {
        // Read ADCs
        throttle_err = read_adc(RIGHT_TRIGGER_IDX, &raw_throttle);
        brake_err = read_adc(LEFT_TRIGGER_IDX, &raw_brake);
        steering_err = read_adc(LEFT_JOYSTICK_X_IDX, &raw_steering);

        // printk("Throttle: %d\nBrake: %d\n Steering: %d\n", raw_throttle, raw_brake, raw_steering);

        // Normalize values and package into input struct
        controller_processed_analog_values new_inputs = {
            .throttle = throttle_err == 0 ? adc_to_throttle_brake(raw_throttle) : 0,
            .brake = brake_err == 0 ? adc_to_throttle_brake(raw_brake) : 100,
            .steering = steering_err == 0 ? adc_to_steering_angle(raw_steering) : 0,
        };

        // Update internal current input values
        k_mutex_lock(&inputs_mutex, K_FOREVER);
        current_inputs = new_inputs;
        k_mutex_unlock(&inputs_mutex);

        k_sleep(K_MSEC(10));
    }
}


int analog_input_get(controller_processed_analog_values *inputs) {
    if (inputs == NULL) {
        return -EINVAL;
    }

    k_mutex_lock(&inputs_mutex, K_FOREVER);
    *inputs = current_inputs;
    k_mutex_unlock(&inputs_mutex);

    return 0;
}

