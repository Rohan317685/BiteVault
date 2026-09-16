#ifndef BITEVAULT_ATECC608_H
#define BITEVAULT_ATECC608_H

#include <stdint.h>

// Driver for U4, the ATECC608A secure element.  This is deliberately a small
// subset of what the part can do: enough to prove the chip is alive on a fresh
// board, read its serial number, and pull hardware entropy out of it.  Key
// generation and signing are not here yet - see firmware/README.md.

#define ATECC_OK           0
#define ATECC_ERR_COMM   (-1) // I2C transfer failed
#define ATECC_ERR_CRC    (-2) // response arrived with a bad CRC
#define ATECC_ERR_STATUS (-3) // device returned a status packet instead of data
#define ATECC_ERR_WAKE   (-4) // no (or wrong) wake response
#define ATECC_ERR_PARAM  (-5)

#define ATECC_I2C_ADDR 0x60 // 7-bit form of the 0xC0 default address

// Wakes the device, reads its revision, then puts it back to sleep.
// `revision` (4 bytes) may be NULL.
int atecc_probe(uint8_t revision[4]);

// 32 bytes of hardware entropy.  Note that an unlocked (factory-fresh) device
// answers with the fixed 0xFFFF0000... test pattern rather than real random
// numbers - that is the chip telling you its config zone is not locked yet.
int atecc_random(uint8_t out[32]);

// The 9-byte factory serial number, from the config zone (always readable).
int atecc_serial(uint8_t out[9]);

// Status byte from the last command that failed with ATECC_ERR_STATUS.
uint8_t atecc_last_status(void);

#endif
