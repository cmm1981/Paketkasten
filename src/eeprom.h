/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EEPROM_H
#define EEPROM_H

int eeprom_init(void);
int eeprom_write_uid_list(void);
void eeprom_clear_uid_list(void);
int eeprom_add_uid(uint8_t *uid, size_t len);
bool eeprom_check_uid(uint8_t *uid, size_t len);

#endif // EEPROM_H