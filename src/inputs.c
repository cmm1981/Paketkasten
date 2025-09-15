/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "powermanager.h"
#include "states.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stdbool.h>

#define HALL_ZU_NODE DT_ALIAS(hallzu)
#if !DT_NODE_HAS_STATUS_OKAY(HALL_ZU_NODE)
#error "Unsupported board: hallzu devicetree alias is not defined"
#endif
static const struct gpio_dt_spec hall_zu =
    GPIO_DT_SPEC_GET_OR(HALL_ZU_NODE, gpios, {0});

#define HALL_P_AUF_NODE DT_ALIAS(hallpauf)
#if !DT_NODE_HAS_STATUS_OKAY(HALL_P_AUF_NODE)
#error "Unsupported board: hallpauf devicetree alias is not defined"
#endif
static const struct gpio_dt_spec hall_p_auf =
    GPIO_DT_SPEC_GET_OR(HALL_P_AUF_NODE, gpios, {0});

#define HALL_B_AUF_NODE DT_ALIAS(hallbauf)
#if !DT_NODE_HAS_STATUS_OKAY(HALL_B_AUF_NODE)
#error "Unsupported board: hallbauf devicetree alias is not defined"
#endif
static const struct gpio_dt_spec hall_b_auf =
    GPIO_DT_SPEC_GET_OR(HALL_B_AUF_NODE, gpios, {0});

#define PAKET_AUF_NODE DT_ALIAS(paketauf)
#if !DT_NODE_HAS_STATUS_OKAY(PAKET_AUF_NODE)
#error "Unsupported board: paketauf devicetree alias is not defined"
#endif
static const struct gpio_dt_spec paket_auf =
    GPIO_DT_SPEC_GET_OR(PAKET_AUF_NODE, gpios, {0});

#define BRIEF_AUF_NODE DT_ALIAS(briefauf)
#if !DT_NODE_HAS_STATUS_OKAY(BRIEF_AUF_NODE)
#error "Unsupported board: briefauf devicetree alias is not defined"
#endif
static const struct gpio_dt_spec brief_auf =
    GPIO_DT_SPEC_GET_OR(BRIEF_AUF_NODE, gpios, {0});

#define JUMPER_NODE DT_ALIAS(jumper)
#if !DT_NODE_HAS_STATUS_OKAY(JUMPER_NODE)
#error "Unsupported board: jumper devicetree alias is not defined"
#endif
static const struct gpio_dt_spec jumper_spec =
    GPIO_DT_SPEC_GET_OR(JUMPER_NODE, gpios, {0});

static struct gpio_callback hall_zu_cb_data;
static struct gpio_callback hall_p_auf_cb_data;
static struct gpio_callback hall_b_auf_cb_data;
static struct gpio_callback paket_auf_cb_data;
static struct gpio_callback brief_auf_cb_data;
static struct gpio_callback jumper_cb_data;

bool input_zu = true;
bool input_p_auf = false;
bool input_b_auf = false;
bool jumper_bit = false;

void hall_change(const struct device *dev, struct gpio_callback *cb,
                 uint32_t pins) {
  if ((pins & BIT(hall_zu.pin)) != 0) {
    input_zu = true;
    input_p_auf = false;
    input_b_auf = false;
  }

  if ((pins & BIT(hall_p_auf.pin)) != 0) {
    input_zu = false;
    input_p_auf = true;
    input_b_auf = false;
  }

  if ((pins & BIT(hall_b_auf.pin)) != 0) {
    input_zu = false;
    input_p_auf = false;
    input_b_auf = true;
  }
}

void taster_cb(const struct device *dev, struct gpio_callback *cb,
               uint32_t pins) {
  printk("Taster gedrückt\n");
  powermanager_wakeup();
  if ((pins & BIT(paket_auf.pin)) != 0) {
    push_command(CMD_OEFFNE_PAKET);
  }

  if ((pins & BIT(brief_auf.pin)) != 0) {
    push_command(CMD_OEFFNE_BRIEF);
  }
}

void jumper_cb(const struct device *dev, struct gpio_callback *cb,
               uint32_t pins) {
    jumper_bit = gpio_pin_get_dt(&jumper_spec);
}

int inputs_init(void) {
  int ret;

  /* Konfiguriere Hall Sensor für Paketkasten zu */
  if (!gpio_is_ready_dt(&hall_zu)) {
    printk("Error: hall_zu device %s is not ready\n", hall_zu.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&hall_zu, GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret, hall_zu.port->name,
           hall_zu.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&hall_zu, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           hall_zu.port->name, hall_zu.pin);
    return 0;
  }

  gpio_init_callback(&hall_zu_cb_data, hall_change, BIT(hall_zu.pin));
  gpio_add_callback(hall_zu.port, &hall_zu_cb_data);

  /* Konfiguriere Hall Sensor für Paketkasten auf */
  if (!gpio_is_ready_dt(&hall_p_auf)) {
    printk("Error: hall_p_auf device %s is not ready\n", hall_p_auf.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&hall_p_auf, GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret,
           hall_p_auf.port->name, hall_p_auf.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&hall_p_auf, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           hall_p_auf.port->name, hall_p_auf.pin);
    return 0;
  }

  gpio_init_callback(&hall_p_auf_cb_data, hall_change, BIT(hall_p_auf.pin));
  gpio_add_callback(hall_p_auf.port, &hall_p_auf_cb_data);

  /* Konfiguriere Hall Sensor für Paketkasten und Briefkasten auf */
  if (!gpio_is_ready_dt(&hall_b_auf)) {
    printk("Error: hall_b_auf device %s is not ready\n", hall_b_auf.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&hall_b_auf, GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret,
           hall_b_auf.port->name, hall_b_auf.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&hall_b_auf, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           hall_b_auf.port->name, hall_b_auf.pin);
    return 0;
  }

  gpio_init_callback(&hall_b_auf_cb_data, hall_change, BIT(hall_b_auf.pin));
  gpio_add_callback(hall_b_auf.port, &hall_b_auf_cb_data);

  /* Konfiguriere Taster für Paketkasten auf */
  if (!gpio_is_ready_dt(&paket_auf)) {
    printk("Error: paket_auf device %s is not ready\n", paket_auf.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&paket_auf, GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret,
           paket_auf.port->name, paket_auf.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&paket_auf, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           paket_auf.port->name, paket_auf.pin);
    return 0;
  }

  gpio_init_callback(&paket_auf_cb_data, taster_cb, BIT(paket_auf.pin));
  gpio_add_callback(paket_auf.port, &paket_auf_cb_data);

  /* Konfiguriere Taster für Paketkasten und Briefkasten auf */
  if (!gpio_is_ready_dt(&brief_auf)) {
    printk("Error: brief_auf device %s is not ready\n", brief_auf.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&brief_auf, GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret,
           brief_auf.port->name, brief_auf.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&brief_auf, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           brief_auf.port->name, brief_auf.pin);
    return 0;
  }

  gpio_init_callback(&brief_auf_cb_data, taster_cb, BIT(brief_auf.pin));
  gpio_add_callback(brief_auf.port, &brief_auf_cb_data);

  /* Konfiguriere Taster für Paketkasten und Briefkasten auf */
  if (!gpio_is_ready_dt(&jumper_spec)) {
    printk("Error: jumper device %s is not ready\n", jumper_spec.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&jumper_spec, GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret,
           jumper_spec.port->name, jumper_spec.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&jumper_spec, GPIO_INT_EDGE_BOTH);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           jumper_spec.port->name, jumper_spec.pin);
    return 0;
  }
  jumper_bit = gpio_pin_get_dt(&jumper_spec);

  gpio_init_callback(&jumper_cb_data, jumper_cb, BIT(jumper_spec.pin));
  gpio_add_callback(jumper_spec.port, &jumper_cb_data);

  return 0;
}

bool *get_kasten_zu(void) { return &input_zu; }

bool *get_paket_auf(void) { return &input_p_auf; }

bool *get_brief_auf(void) { return &input_b_auf; }

bool get_jumper_bit(void) { return jumper_bit; }