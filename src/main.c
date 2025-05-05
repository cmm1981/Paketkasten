/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include "motor.h"
#include "inputs.h"
#include "states.h"


/* Mainloob will sleep for 100ms */
#define SLEEP_TIME_MS   100

/* The devicetree node identifier for the "led0"(grün), "led1"(rot) and "vdden" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define VDDEN_NODE DT_ALIAS(vdden)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec vdd_en = GPIO_DT_SPEC_GET(VDDEN_NODE, gpios);

static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void uart_cb(const struct device *dev, void *user_data)
{
    uint8_t c;
    while (uart_fifo_read(dev, &c, 1)) {
        switch(c)
		{
			case 'o':
				push_command(CMD_OEFFNE_BRIEF);
				break;

			case 'p':
				push_command(CMD_OEFFNE_PAKET);
				break;
		}
    }
}

int main(void)
{
	int ret;

	if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return 0;
    }

    uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
    uart_irq_rx_enable(uart_dev);

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&led2) || !gpio_is_ready_dt(&vdd_en)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&vdd_en, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	/* Schalte Versorgung für Peripherie ein */
	ret = gpio_pin_set_dt(&vdd_en, 1);
	if (ret < 0) {
		return 0;
	}

	/* Schalte grüne LED ein*/
	ret = gpio_pin_set_dt(&led, 1);
	if (ret < 0) {
		return 0;
	}

	/* Schalte rote LED aus*/
	ret = gpio_pin_set_dt(&led2, 0);
	if (ret < 0) {
		return 0;
	}

	motor_init();
	if (ret < 0) {
		return 0;
	}

	inputs_init();
	if (ret < 0) {
		return 0;
	}

	while (1) 
	{
		state_machine();

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
