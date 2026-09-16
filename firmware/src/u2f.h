#ifndef BITEVAULT_U2F_H
#define BITEVAULT_U2F_H

#include <stdint.h>

// CTAP1 / U2F raw message layer (what arrives in a CTAPHID_MSG).
//
// Handles the ISO 7816 APDU framing and answers U2F_VERSION.  Registration and
// authentication need ECDSA P-256 keys out of the ATECC608A and are not
// implemented yet; they return the status words a host expects from a key that
// has nothing registered, so browsers fail cleanly instead of hanging.

// Writes the response APDU (payload + 2-byte status word) into `resp` and
// returns its length, or 0 if it does not fit.
uint16_t u2f_handle_apdu(const uint8_t *req, uint16_t req_len,
                         uint8_t *resp, uint16_t resp_max);

#endif
