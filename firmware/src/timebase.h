#ifndef BITEVAULT_TIMEBASE_H
#define BITEVAULT_TIMEBASE_H

#include <stdint.h>

// Free-running 1 kHz tick off SysTick.  Everything that needs to measure time
// (LED patterns, button debounce, CTAPHID transaction timeouts, ATECC608A
// command polling) uses this instead of blocking delays.
void timebase_init(void);

// Milliseconds since timebase_init().  Wraps after ~49 days; compare with
// subtraction (now - then) so the wrap is harmless.
uint32_t millis(void);

#endif
