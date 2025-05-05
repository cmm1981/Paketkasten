/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "states.h"
#include "motor.h"
#include "inputs.h"

typedef enum {
    STATE_GESCHLOSSEN,
    STATE_PAKET_OFFEN,
    STATE_BRIEF_OFFEN,
    STATE_PAKET_GESPERRT,
    STATE_PAKET_SICHER_OFFEN,
    STATE_WARTEN,
} state_t;

typedef struct {
    bool *condition;
    state_t next_state;
} warten_t;

static state_t current_state = STATE_GESCHLOSSEN; // Initialzustand
static warten_t warten;

// FIFO für die Befehle
K_PIPE_DEFINE(command_pipe, sizeof(command_t)*10,4);

void push_command(command_t command) {
    k_pipe_write(&command_pipe, (uint8_t *)&command, sizeof(command_t), K_NO_WAIT);
}

void goto_warten(bool *condition, state_t next)
{
    warten.condition = condition;
    warten.next_state = next;
    current_state = STATE_WARTEN;
}

// Funktion zum Verarbeiten von Befehlen in der FIFO
void process_commands() 
{
    command_t cmd;
    if(k_pipe_read(&command_pipe, (uint8_t *)&cmd, sizeof(command_t), K_NO_WAIT) == sizeof(command_t)) 
    {
        switch (cmd) 
        {
            case CMD_OEFFNE_PAKET:
                goto_warten(get_paket_auf(),STATE_PAKET_OFFEN);
                motor_set(MOTOR_ZUR,3,get_paket_auf());
                break;

            case CMD_OEFFNE_BRIEF:
            goto_warten(get_brief_auf(),STATE_BRIEF_OFFEN);
                motor_set(MOTOR_ZUR,3,get_brief_auf());
                break;

            default:
                printk("Unbekannter Befehl\n");
                break;
        }
    }
}

void state_machine(void) {
    switch (current_state) {
        case STATE_GESCHLOSSEN:
            // Logik für den Zustand "geschlossen"
            process_commands(); // Ankommende Befehle verarbeiten
            break;

        case STATE_PAKET_OFFEN:
            // Logik für den Zustand "paket_offen"
            motor_set(MOTOR_VOR,3,get_kasten_zu());
            k_pipe_reset(&command_pipe); //Pipe leeren
            goto_warten(get_kasten_zu(),STATE_GESCHLOSSEN);
            break;

        case STATE_BRIEF_OFFEN:
            // Logik für den Zustand "brief_offen"
            motor_set(MOTOR_VOR,3,get_kasten_zu());
            k_pipe_reset(&command_pipe); //Pipe leeren
            goto_warten(get_kasten_zu(),STATE_GESCHLOSSEN);
            break;

        case STATE_PAKET_GESPERRT:
            // Logik für den Zustand "paket_gesperrt"
            printk("State: paket_gesperrt\n");
            break;

        case STATE_PAKET_SICHER_OFFEN:
            // Logik für den Zustand "paket_sicher_offen"
            printk("State: paket_sicher_offen\n");
            break;

        case STATE_WARTEN:
            if(*(warten.condition))
            {
                current_state = warten.next_state;
            }
            break;

        
        default:
            // Unbekannter Zustand
            printk("Unbekannter Zustand\n");
            break;
    }
}
