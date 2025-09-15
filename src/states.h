
/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STATES_H
#define STATES_H

typedef enum {
  CMD_OEFFNE_PAKET,
  CMD_OEFFNE_BRIEF,
} command_t;

void push_command(command_t command);
void state_machine(void);

#endif // STATES_H