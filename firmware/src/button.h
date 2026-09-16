#ifndef BITEVAULT_BUTTON_H
#define BITEVAULT_BUTTON_H

#include <stdint.h>

// SW1, debounced.  FIDO calls this the "user presence" button: nothing that
// signs anything should happen without a fresh press of it.
void button_init(void);
void button_task(void);

// True while the button is physically held down.
int button_is_down(void);

// True if the button went down in the last `max_age_ms` and that press has not
// been consumed yet.  Consumes the press when it returns true, so one press
// authorises exactly one operation.
int button_take_press(uint32_t max_age_ms);

#endif
