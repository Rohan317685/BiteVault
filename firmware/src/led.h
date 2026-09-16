#ifndef BITEVAULT_LED_H
#define BITEVAULT_LED_H

#include <stdint.h>

// D1 is the only output the user can see, so it carries all of the device
// state.  Patterns are non-blocking: set one and keep calling led_task().
typedef enum {
	LED_OFF,      // dark
	LED_IDLE,     // short blip every few seconds: powered, USB not up yet
	LED_READY,    // solid on: enumerated and idle
	LED_BUSY,     // fast flicker: handling a CTAPHID transaction
	LED_TOUCH,    // slow even blink: waiting for the user to press SW1
	LED_ERROR,    // urgent blink: self-test failed
} led_pattern_t;

void led_init(void);
void led_set(led_pattern_t pattern);

// Show `pattern` for `ms`, then fall back to whatever led_set() last held.
void led_flash(led_pattern_t pattern, uint16_t ms);

void led_task(void);

#endif
