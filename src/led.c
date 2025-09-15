/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* The devicetree node identifier for the "led0"(grün) and "led1"(rot) alias. */
#define LEDGREEN_NODE DT_ALIAS(ledgreen)
#define LEDRED_NODE DT_ALIAS(ledred)

static const struct gpio_dt_spec led_green =
    GPIO_DT_SPEC_GET(LEDGREEN_NODE, gpios);
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LEDRED_NODE, gpios);

int led_init(void) {
  int ret;

  if (!gpio_is_ready_dt(&led_green) || !gpio_is_ready_dt(&led_red)) {
    return -1;
  }

  ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return -1;
  }

  ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return -1;
  }

  /* Schalte grüne LED ein*/
  ret = gpio_pin_set_dt(&led_green, 1);
  if (ret < 0) {
    return -1;
  }

  /* Schalte rote LED aus*/
  ret = gpio_pin_set_dt(&led_red, 0);
  if (ret < 0) {
    return -1;
  }
  return 0;
}

void led_green_on(void) {
  /* Schalte grüne LED ein*/
  gpio_pin_set_dt(&led_green, 1);
}

void led_green_off(void) {
  /* Schalte grüne LED ein*/
  gpio_pin_set_dt(&led_green, 0);

}

void led_green_toggle(void) {
  if(gpio_pin_get_dt(&led_green)) {
     gpio_pin_set_dt(&led_green, 0);
  } else {
     gpio_pin_set_dt(&led_green, 1);
  }
}

void led_red_on(void) {
  /* Schalte grüne LED ein*/
  gpio_pin_set_dt(&led_red, 1);
}

void led_red_off(void) {
  /* Schalte grüne LED ein*/
  gpio_pin_set_dt(&led_red, 0);
}