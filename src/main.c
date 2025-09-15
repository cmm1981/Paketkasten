/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include "eeprom.h"
#include "inputs.h"
#include "led.h"
#include "motor.h"
#include "powermanager.h"
#include "rfid.h"
#include "states.h"
#include <zephyr/debug/thread_analyzer.h>


/* Mainloob will sleep for 100ms */
#define SLEEP_TIME_MS 100

static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void uart_cb(const struct device *dev, void *user_data) {
  uint8_t c;
  powermanager_wakeup();
  /* Read all available data from the UART FIFO */
  while (uart_fifo_read(dev, &c, 1)) {
    switch (c) {
    case 'o':
      push_command(CMD_OEFFNE_BRIEF);
      break;

    case 'p':
      push_command(CMD_OEFFNE_PAKET);
      break;
    }
  }
}

int main(void) {
  int ret;

  if (!device_is_ready(uart_dev)) {
    printk("UART device not ready\n");
    return 0;
  }

  uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
  uart_irq_rx_enable(uart_dev);

  ret = led_init();
  if (ret < 0) {
    return 0;
  }

  ret = motor_init();
  if (ret < 0) {
    return 0;
  }

  ret = inputs_init();
  if (ret < 0) {
    return 0;
  }

  ret = eeprom_init();
  if (ret < 0) {
    return 0;
  }

  powermanager_init();

  rfid_init();

  while (1) {
    state_machine();

    k_msleep(SLEEP_TIME_MS);

    //thread_analyzer_print(0);
  }
  return 0;
}
