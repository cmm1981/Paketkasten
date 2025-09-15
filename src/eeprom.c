/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/eeprom.h>
#include <zephyr/kernel.h>

#define EEPROM_NODE DT_NODELABEL(eeprom0)

#define EEPROM_PAGE_SIZE 64

static const struct device *eeprom_dev;

#define UID_LIST_LEN 6
#define UID_MAX_LEN 10

static struct uid_list {
  uint8_t uid[UID_LIST_LEN][UID_MAX_LEN];
  size_t uid_count;
} uids;

BUILD_ASSERT(sizeof(struct uid_list) <= EEPROM_PAGE_SIZE,
             "struct uid_list is to big for one EEPROM page");

int eeprom_init(void) {

  int ret;

  eeprom_dev = DEVICE_DT_GET(EEPROM_NODE);

  if (!device_is_ready(eeprom_dev)) {
    printk("AT25 EEPROM not ready!\n");
    return -1;
  }

  ret = eeprom_read(eeprom_dev, 0, &uids, sizeof(uids));
  if (ret < 0) {
    printk("Read failed: %d\n", ret);
    return ret;
  }

  return 0;
}

int eeprom_write_uid_list(void) {

  int ret;

  ret = eeprom_write(eeprom_dev, 0, &uids, sizeof(uids));
  if (ret < 0) {
    printk("Write failed: %d\n", ret);
    return ret;
  }

  for (size_t i = 0; i < uids.uid_count; i++) {
    printk("UID[%d]: ", i);
    for (size_t j = 0; j < UID_MAX_LEN; j++) {
      printk("%02X ", uids.uid[i][j]);
    }
    printk("\n");
  }

  k_msleep(10);

  return 0;
}

void eeprom_clear_uid_list(void) { memset(&uids, 0, sizeof(uids)); }

int eeprom_add_uid(uint8_t *uid, size_t len) {

  if (len > UID_MAX_LEN) {
    printk("Wrong UID size! Max 10 bytes allowed");
    return -EINVAL;
  }

  if (uids.uid_count >= UID_LIST_LEN) {
    printk("UID List is full");
    return -ENOMEM;
  }


  memcpy(uids.uid[uids.uid_count], uid, len);
  memset(&uids.uid[uids.uid_count][len], 0, UID_MAX_LEN - len);
  uids.uid_count += 1;

  return 0;
}

bool eeprom_check_uid(uint8_t *uid, size_t len) {

  for (uint8_t i = 0; i < uids.uid_count; i++) {
    if (memcmp(uids.uid[i], uid, len) == 0) {
      return true;
    }
  }
  return false;
}