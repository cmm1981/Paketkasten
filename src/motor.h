/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#ifndef MOTOR_H
#define MOTOR_H

typedef enum { MOTOR_STOP, MOTOR_VOR, MOTOR_ZUR } motor_richtung_t;

int motor_init(void);
void motor_set(motor_richtung_t richtung, uint8_t timeout_s, bool *stop);

#endif // MOTOR_H
