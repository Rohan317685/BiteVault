#ifndef BITEVAULT_BOARD_H
#define BITEVAULT_BOARD_H

#include "ch32fun.h"

// ---------------------------------------------------------------------------
// BiteVault rev A pin map.
//
// Taken from the KiCad netlist (Production/netlist.ipc), U1 = CH32V203G6U6:
//
//   pin 6  PA0  -> SW1 pin 1, SW1 pin 2 to GND   (user presence button)
//   pin 7  PA1  -> D1 cathode, anode via R5 to 3V3 (status LED, sinks current)
//   pin 19 PA11 -> USB D- through R7/U2
//   pin 20 PA12 -> USB D+ through R6/U2
//   pin 21 PA13 -> TP1 (SWDIO)
//   pin 22 PA14 -> TP2 (SWCLK)
//   pin 27 PB6  -> SCL, pulled up by R4 (ATECC608A U4 pin 6)
//   pin 28 PB7  -> SDA, pulled up by R3 (ATECC608A U4 pin 5)
//   pin 2/3 PD0/PD1 -> Y1 crystal
// ---------------------------------------------------------------------------

#define PIN_LED     PA1
#define PIN_BUTTON  PA0
#define PIN_SCL     PB6
#define PIN_SDA     PB7

// D1's anode sits at 3V3, so the pin sinks the LED current: low = lit.
#define LED_ON_LEVEL   0
// SW1 shorts PA0 to ground, so the pin needs a pull-up: low = pressed.
#define BUTTON_DOWN_LEVEL 0

#endif
