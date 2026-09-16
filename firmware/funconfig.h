#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// BiteVault runs the CH32V203G6U6 from the on-board crystal Y1 (8 MHz, loaded
// by C9/C10).  8 MHz * 12 = 96 MHz, which the USBFS peripheral divides by 2 to
// get the 48 MHz it needs.  USB only works at 48/96/144 MHz on this part, so if
// you ever fit a different crystal you must re-pick FUNCONF_PLL_MULTIPLIER to
// land on one of those.
#define FUNCONF_USE_HSE             1
#define FUNCONF_USE_HSI             0
#define FUNCONF_PLL_MULTIPLIER      12
#define FUNCONF_SYSTEM_CORE_CLOCK   96000000

// The board is regulated to 3.3 V by U3, USB pull-ups are internal.
#define FUNCONF_USE_5V_VDD          0

// printf() goes out over the WCH-Link debug channel (`make monitor`), not a UART.
#define FUNCONF_USE_DEBUGPRINTF     1
#define FUNCONF_DEBUG_HARDFAULT     1

#define FUNCONF_ENABLE_HPE          0
#define FUNCONF_SYSTICK_USE_HCLK    1

#endif
