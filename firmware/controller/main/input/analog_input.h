#pragma once

#include <stdint.h>


typedef struct {
    uint8_t throttle;
    uint8_t brake;
    int16_t steering;
} controller_processed_analog_values;


int analog_input_init(void);


// ADC sampling and normalization thread
void analog_input_start(void* arg1, void* arg2, void* arg3);


int analog_input_get(controller_processed_analog_values *inputs);