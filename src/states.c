/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdbool.h>
#include <zephyr/kernel.h>
#include "states.h"
#include "inputs.h"
#include "led.h"
#include "motor.h"
#include "powermanager.h"
#include "rfid.h"

typedef enum {
  STATE_GESCHLOSSEN,
  STATE_PAKET_OFFEN,
  STATE_BRIEF_OFFEN,
  STATE_PAKET_GESPERRT,
  STATE_PAKET_SICHER_OFFEN,
  STATE_WARTEN,
  STATE_RFID_PROGRAMMIEREN,
} state_t;

typedef struct {
  bool *condition;
  state_t next_state;
} warten_t;

static state_t current_state = STATE_GESCHLOSSEN; // Initialzustand
static warten_t warten;

// FIFO für die Befehle
K_PIPE_DEFINE(command_pipe, sizeof(command_t), 2);

void push_command(command_t command) {
  k_pipe_write(&command_pipe, (uint8_t *)&command, sizeof(command_t),
               K_NO_WAIT);
}

void goto_warten(bool *condition, state_t next, bool led) {
  warten.condition = condition;
  warten.next_state = next;
  current_state = STATE_WARTEN;
  // LED rot einschalten
  if (led) {
    led_red_on();
  }
}

static void timeout_handler(struct k_timer *timer_id);

K_TIMER_DEFINE(timer, timeout_handler, NULL);
static bool timeout = false;

void timeout_handler(struct k_timer *timer_id) { timeout = true; }

int start_timer(uint32_t time_ms) {
  timeout = false;
  k_timer_user_data_set(&timer, NULL);
  k_timer_start(&timer, K_MSEC(time_ms), K_NO_WAIT);

  return 0;
}

// Funktion zum Verarbeiten von Befehlen in der FIFO
void process_commands() {
  command_t cmd;
  if (k_pipe_read(&command_pipe, (uint8_t *)&cmd, sizeof(command_t),
                  K_NO_WAIT) == sizeof(command_t)) {
    switch (cmd) {
    case CMD_OEFFNE_PAKET:
      if (current_state == STATE_GESCHLOSSEN) {
        goto_warten(get_paket_auf(), STATE_PAKET_OFFEN, false);
        motor_set(MOTOR_ZUR, 3, get_paket_auf());
      } else {
        // STATE_PAKET_GESPERRT: Das Paket muss erst vom Besitzer herausgenommen
        // werden
        start_timer(1000);
        goto_warten(&timeout, STATE_PAKET_GESPERRT, true);
      }
      break;

    case CMD_OEFFNE_BRIEF:
      goto_warten(get_brief_auf(), STATE_BRIEF_OFFEN, false);
      motor_set(MOTOR_ZUR, 3, get_brief_auf());
      break;

    default:
      printk("Unbekannter Befehl\n");
      break;
    }
  } else {
    powermanager_check();
  }
}

void process_rfid_programming(void) {

}

void state_machine(void) {
  switch (current_state) {
  case STATE_GESCHLOSSEN:
    // Logik für den Zustand "geschlossen"
    if (get_jumper_bit() == 0 ) {
		current_state = STATE_RFID_PROGRAMMIEREN;
		break;
    }
    process_commands(); // Ankommende Befehle verarbeiten
    break;

  case STATE_PAKET_OFFEN:
    // Logik für den Zustand "paket_offen"
    powermanager_trigger();
    motor_set(MOTOR_VOR, 3, get_kasten_zu());
    k_pipe_reset(&command_pipe); // Pipe leeren
    goto_warten(get_kasten_zu(), STATE_PAKET_GESPERRT, false);
    break;

  case STATE_BRIEF_OFFEN:
    // Logik für den Zustand "brief_offen"
    powermanager_trigger();
    motor_set(MOTOR_VOR, 3, get_kasten_zu());
    k_pipe_reset(&command_pipe); // Pipe leeren
    goto_warten(get_kasten_zu(), STATE_GESCHLOSSEN, false);
    break;

  case STATE_PAKET_GESPERRT:
    // Logik für den Zustand "paket_gesperrt"
    process_commands(); // Ankommende Befehle verarbeiten
    break;

  case STATE_PAKET_SICHER_OFFEN:
    // Logik für den Zustand "paket_sicher_offen"
    printk("State: paket_sicher_offen\n");
    break;

  case STATE_WARTEN:
    powermanager_trigger();
    if (*(warten.condition)) {
      current_state = warten.next_state;
      led_red_off();
    }
    break;

  case STATE_RFID_PROGRAMMIEREN:
    rfid_set_programming_mode();
    powermanager_trigger();
    led_green_toggle();
    start_timer(100);
    goto_warten(&timeout, STATE_RFID_PROGRAMMIEREN, false);
    if (get_jumper_bit() == 1 ) {
	current_state = STATE_GESCHLOSSEN;
	rfid_set_normal_mode();
	led_green_on();
    }
    break;

  default:
    // Unbekannter Zustand
    printk("Unbekannter Zustand\n");
    break;
  }
}
