/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/pm/device_runtime.h>
#include <dk_buttons_and_leds.h>

#include "../include/rc4_controller.h"


LOG_MODULE_REGISTER(esb_ptx, CONFIG_ESB_PTX_APP_LOG_LEVEL);


static bool ready = true;
static struct esb_payload rx_payload;
static const uint8_t PIPE_ADDR = 0;
// struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(PIPE_ADDR, ... );  // Should create payloads on the fly before sending.

#define _RADIO_SHORTS_COMMON                                                   \
	(RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk |         \
	 RADIO_SHORTS_ADDRESS_RSSISTART_Msk |                                  \
	 RADIO_SHORTS_DISABLED_RSSISTOP_Msk)

	 

void event_handler(struct esb_evt const *event)
{
	ready = true;

	switch (event->evt_id) {
	case ESB_EVENT_TX_SUCCESS:
		LOG_DBG("TX SUCCESS EVENT %u attempts", event->tx_attempts);
		break;
	case ESB_EVENT_TX_FAILED:
		LOG_DBG("TX FAILED EVENT");
		break;
	case ESB_EVENT_RX_RECEIVED:
		while (esb_read_rx_payload(&rx_payload) == 0) {
			LOG_DBG("Packet received, len %d : "
				"0x%02x, 0x%02x, 0x%02x, 0x%02x, "
				"0x%02x, 0x%02x, 0x%02x, 0x%02x",
				rx_payload.length, rx_payload.data[0],
				rx_payload.data[1], rx_payload.data[2],
				rx_payload.data[3], rx_payload.data[4],
				rx_payload.data[5], rx_payload.data[6],
				rx_payload.data[7]);
		}
		break;
	}
}


int esb_initialize(void)
{
	int err;
	uint8_t base_addr_0[4] = {0x07, 0x1A, 0xE7, 0xB4};
	uint8_t base_addr_1[4] = {0xA0, 0x72, 0xB1, 0xAA};
	uint8_t addr_prefix[8] = {0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87};

	struct esb_config config = ESB_DEFAULT_CONFIG;

	config.protocol = ESB_PROTOCOL_ESB_DPL;
	config.retransmit_delay = 600;
	config.bitrate = ESB_BITRATE_2MBPS;
	config.event_handler = event_handler;
	config.mode = ESB_MODE_PTX;
	config.selective_auto_ack = true;
	if (IS_ENABLED(CONFIG_ESB_FAST_SWITCHING)) {
		config.use_fast_ramp_up = true;
	}

	err = esb_init(&config);

	if (err) {
		return err;
	}

	err = esb_set_base_address_0(base_addr_0);
	if (err) {
		return err;
	}

	err = esb_set_base_address_1(base_addr_1);
	if (err) {
		return err;
	}

	err = esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));
	if (err) {
		return err;
	}

	return 0;
}


static void leds_update(uint8_t value)
{
	uint32_t leds_mask =
		(!(value % 8 > 0 && value % 8 <= 4) ? DK_LED1_MSK : 0) |
		(!(value % 8 > 1 && value % 8 <= 5) ? DK_LED2_MSK : 0) |
		(!(value % 8 > 2 && value % 8 <= 6) ? DK_LED3_MSK : 0) |
		(!(value % 8 > 3) ? DK_LED4_MSK : 0);

	dk_set_leds(leds_mask);
}


int main(void)
{
	int err;

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs initialization failed, err %d", err);
		return 0;
	}

	err = esb_initialize();
	if (err) {
		LOG_ERR("ESB initialization failed, err %d", err);
		return 0;
	}

	LOG_INF("Initialization complete");

	// Regular controller state transmissions (100Hz)
	while (1) {
		rc4_controller_state state = rc4_controller_get_state();  // Read ADC and process
		struct esb_payload tx_payload = package_esb_command(state);
	
		esb_flush_tx();
		tx_payload.noack = false;
		err = esb_write_payload(&tx_payload);
	
		if (err) {
			LOG_WRN("ESB TX failed: %d", err);
		}
	
		k_msleep(10);
	}
}








// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/adc.h>
// #include <zephyr/sys/printk.h>


// #define THROTTLE_ADC_MIN  640
// #define THROTTLE_ADC_MAX  1650


// static const struct adc_dt_spec adc =
//     ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

// static int16_t sample_buffer;


// uint8_t adc_to_throttle(int16_t raw)
// {
//     if (raw <= THROTTLE_ADC_MIN) {
//         return 100;
//     }

//     if (raw >= THROTTLE_ADC_MAX) {
//         return 0;
//     }

//     return (uint8_t)(
//         ((int32_t)(raw - THROTTLE_ADC_MIN) * 100) /
//         (THROTTLE_ADC_MAX - THROTTLE_ADC_MIN)
//     );
// }


// int main(void)
// {
//     int err;

//     printk("\n\nPROGRAM STARTED\n");

//     if (!adc_is_ready_dt(&adc)) {
//         printk("ERROR: ADC device not ready\n");

//         while (1) {
//             k_sleep(K_FOREVER);
//         }
//     }

//     printk("ADC device ready\n");

//     err = adc_channel_setup_dt(&adc);

//     if (err < 0) {
//         printk("ERROR: ADC setup failed: %d\n", err);

//         while (1) {
//             k_sleep(K_FOREVER);
//         }
//     }

//     printk("ADC channel configured\n");

//     struct adc_sequence sequence = {
//         .buffer = &sample_buffer,
//         .buffer_size = sizeof(sample_buffer),
//     };

//     err = adc_sequence_init_dt(&adc, &sequence);

//     if (err < 0) {
//         printk("ERROR: sequence init failed: %d\n", err);

//         while (1) {
//             k_sleep(K_FOREVER);
//         }
//     }

//     printk("ADC monitoring started\n");

//     while (1) {

//         err = adc_read_dt(&adc, &sequence);

//         if (err < 0) {
//             printk("ADC read error: %d\n", err);
//         } else {

//             int16_t raw = sample_buffer;
//             uint8_t throttle = adc_to_throttle(sample_buffer);
			
//             printk("RAW = %d | Throttle: %d%% \n", raw, throttle);
            
//         }

//         k_msleep(500);
//     }
// }