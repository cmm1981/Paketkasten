/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rfid.h>
#include <zephyr/kernel.h>
#include <zephyr/rfid/iso14443.h>
#include "states.h"
#include "eeprom.h"
#include "powermanager.h"

#define RFID_MAIN_STACK_SIZE 1024
#define RFID_MAIN_PRIORITY 5

#define RFID_NODE DT_ALIAS(rfid)
static const struct device *rfid_dev = DEVICE_DT_GET(RFID_NODE);

K_THREAD_STACK_DEFINE(rfid_main_stack, RFID_MAIN_STACK_SIZE);
static struct k_thread rfid_main_data;
k_tid_t rfid_main_id;

struct my_rfid_iso14443a_info {
  uint8_t atqa[RFID_ISO14443A_MAX_ATQA_LEN];
  uint8_t uid[RFID_ISO14443A_MAX_UID_LEN];
  uint8_t uid_len;
  uint8_t sak;
} info;

static bool programming = false;

static void rfid_main(void *p1, void *p2, void *p3) {
  struct rfid_property props[2];

  props[0].type = RFID_PROP_SLEEP;
  /* refid_set_properties shall block until a tag is detected*/
  props[0].timeout_us = UINT32_MAX;

  props[1].type = RFID_PROP_RESET;

  while (1) {
    rfid_set_properties(rfid_dev, &props[0], 1);

    if (props[0].status != 0) {
      /** Workaround for CR95HF:
       * If the Tag is removed while rfid_iso14443a_sdd is not finished, the
       * CR95HF seems to be in an internal state where Sleep Mode is not
       * possible any longer. To leave that state, you have to re-represent the
       * tag and wait for the end of communication or you have to reset the
       * device.
       */
      rfid_set_properties(rfid_dev, &props[1], 1);
      continue;
    }

    powermanager_wakeup();

    rfid_load_protocol(rfid_dev, RFID_PROTO_ISO14443A,
                       RFID_MODE_INITIATOR | RFID_MODE_TX_106 |
                           RFID_MODE_RX_106);

    memset(&info, 0, sizeof(info));

    if (rfid_iso14443a_request(rfid_dev, info.atqa, true) == 0) {
      if (rfid_iso14443a_sdd(rfid_dev, (struct rfid_iso14443a_info *)&info) ==
          0) {

		if(eeprom_check_uid(info.uid, info.uid_len)) {
			if(programming == false)
			{
				push_command(CMD_OEFFNE_BRIEF);
			}
		} else {
			if(programming == true) {
				eeprom_add_uid(info.uid, info.uid_len);
			}
		}
		k_sleep(K_SECONDS(2));
        }
      }
    }
}

void rfid_init(void) {
  /* Starte rfid_main */
  rfid_main_id = k_thread_create(
      &rfid_main_data, rfid_main_stack, K_THREAD_STACK_SIZEOF(rfid_main_stack),
      rfid_main, NULL, NULL, NULL, RFID_MAIN_PRIORITY, 0, K_NO_WAIT);
}

void rfid_set_programming_mode(void) {
	if(programming == false) {
		eeprom_clear_uid_list();
		printk("programming mode\n");
		programming = true;
	}
}

void rfid_set_normal_mode(void) {
	if(programming == true) {
		programming = false;
		printk("normal mode\n");
		eeprom_write_uid_list();
	}
}