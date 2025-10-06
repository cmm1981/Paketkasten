/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef POWERMANAGER_H
#define POWERMANAGER_H

void powermanager_init(void);
void powermanager_trigger(void);
void powermanager_check(void);
void powermanager_wakeup(void);

#endif // POWERMANAGER_H
