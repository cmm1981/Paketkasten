/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_H
#define LED_H

/**
 * @brief Initialisiere die LEDs
 *
 * Diese Funktion konfiguriert die LEDs und setzt ihren
 * Anfangszustand.
 *
 * @return 0 bei Erfolg, -1 bei einem Fehler.
 */
int led_init(void);

/**
 * @brief Schalte die grüne LED ein
 *
 * Diese Funktion schaltet die grüne LED ein.
 */
void led_green_on(void);

/**
 * @brief Schalte die grüne LED aus
 *
 * Diese Funktion schaltet die grüne LED aus.
 */
void led_green_off(void);

/**
 * @brief Schalte die grüne LED um
 *
 * Diese Funktion schaltet die grüne LED um. Wenn sie aus ist, wird sie eingeschaltet.
 * Ist sie eingeschaltet wird sie ausgeschaltet
 */
void led_green_toggle(void);

/**
 * @brief Schalte die rote LED ein
 *
 * Diese Funktion schaltet die rote LED ein.
 */
void led_red_on(void);

/**
 * @brief Schalte die rote LED aus
 *
 * Diese Funktion schaltet die rote LED aus.
 */
void led_red_off(void);

#endif // LED_H