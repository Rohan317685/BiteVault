#ifndef BITEVAULT_I2C_H
#define BITEVAULT_I2C_H

#include <stdint.h>

// Hardware I2C1 master on PB6/PB7, the bus the ATECC608A (U4) hangs off.
// R3/R4 provide the 4.7k pull-ups, so no internal pull-ups are used here.

#define I2C_OK        0
#define I2C_ERR_NACK  (-1) // no device acknowledged (the ATECC does this while asleep or busy)
#define I2C_ERR_TIMEO (-2) // the peripheral never reported the flag we waited for
#define I2C_ERR_BUS   (-3) // arbitration lost / bus error

void i2c_init(uint32_t speed_hz);

int i2c_write(uint8_t addr7, const uint8_t *data, uint16_t len, int send_stop);
int i2c_read(uint8_t addr7, uint8_t *data, uint16_t len);

// Address-only transaction; I2C_OK means something ACKed at that address.
int i2c_probe(uint8_t addr7);

// Hold SDA low for `low_us` with the peripheral disabled, then release it and
// hand the pin back to I2C1.  The ATECC608A has no reset pin: this pulse is how
// you wake it from sleep.
void i2c_wake_pulse(uint16_t low_us);

#endif
