#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

#include "input/analog_input.h"
#include "input/buttons.h"
#include "wireless/wireless.h"


#define ANALOG_STACK_SIZE 1024
#define ANALOG_THREAD_PRIORITY 5


K_THREAD_STACK_DEFINE(analog_input_stack, ANALOG_STACK_SIZE);
static struct k_thread analog_input_thread;

int main(void) {
    int err = analog_input_init();

    if (err < 0) {
        printk("ADC initialization failed: %d\n", err);
        return 0;
    }

    k_thread_create(
        &analog_input_thread,
        analog_input_stack,
        K_THREAD_STACK_SIZEOF(analog_input_stack),
        analog_input_start,
        NULL, NULL, NULL,
        ANALOG_THREAD_PRIORITY,
        0,
        K_NO_WAIT
    );

    int analog_err = 0;
    controller_processed_analog_values input_values = {0};
    while (1) {
        analog_err = analog_input_get(&input_values);
        if (analog_err) printk("Error while retrieving input values");
        // printk("Throttle: %d\nBrake: %d\n Steering: %d\n", input_values.throttle, input_values.brake, input_values.steering);
        k_sleep(K_MSEC(10));
    }


    // ...

    return 0;
}