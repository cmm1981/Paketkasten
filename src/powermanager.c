/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "led.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* Todo: Die Zephyr Treiber unterstützen aktuell noch keine Powermodes für den
   STM32L100 sodass er nicht schlafen geht. Die Treiber müssen erweitert werden
   und die Powermodes im DTS aktiviert werden. Dann geht auch der STM32L100 in
   den Sleepmode Aktuell wird nur die externe peripherie abgeschaltet */

/* The devicetree node identifier for "vdden" alias. */
#define VDDEN_NODE DT_ALIAS(vdden)

static const struct gpio_dt_spec vdd_en = GPIO_DT_SPEC_GET(VDDEN_NODE, gpios);

static struct k_timer sleep_timer;
static volatile enum { RUNNING, SLEEPING } system_state = RUNNING;
static struct k_thread *sleep_thread = NULL;

static void sleep_timer_callback(struct k_timer *timer_id) {
  system_state = SLEEPING;
}

void powermanager_trigger(void) {
  k_timer_start(&sleep_timer, K_SECONDS(5), K_NO_WAIT);
  system_state = RUNNING;
}

void powermanager_check(void) {
  if (system_state == SLEEPING) {
    printk("System entering sleep mode\n");
    /* Schalte Versorgung für Peripherie aus */
    /* This will turn off the VDDEN pin */
    /* and disable the power to the peripherals */
    gpio_pin_set_dt(&vdd_en, 0);
    /* Set the system to sleep */
    /* This will block the current thread until woken up */
    /* by powermanager_wakeup() */
    sleep_thread = k_current_get();
    k_sleep(K_FOREVER);
  }
}

void powermanager_init(void) {
  int ret;

  /* Konfiguriere VDDEN Pin */
  /* Der Pin ist als Ausgang konfiguriert und wird auf HIGH gesetzt */
  if (!gpio_is_ready_dt(&vdd_en)) {
    return;
  }

  ret = gpio_pin_configure_dt(&vdd_en, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return;
  }

  /* Schalte Versorgung für Peripherie ein */
  ret = gpio_pin_set_dt(&vdd_en, 1);
  if (ret < 0) {
    return;
  }

  k_timer_init(&sleep_timer, sleep_timer_callback, NULL);
  powermanager_trigger();
}

void powermanager_wakeup(void) {
  system_state = RUNNING;
  if (sleep_thread != NULL) {
    /* Schalte Versorgung für Peripherie ein */
    gpio_pin_set_dt(&vdd_en, 1);

    k_wakeup(sleep_thread);
    sleep_thread = NULL;
    printk("System waking up\n");
  }
}