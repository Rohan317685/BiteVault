#include "u2f.h"

// Instructions (FIDO U2F raw message format, section 3).
#define U2F_INS_REGISTER     0x01
#define U2F_INS_AUTHENTICATE 0x02
#define U2F_INS_VERSION      0x03

// Status words.
#define SW_NO_ERROR                 0x9000
#define SW_CONDITIONS_NOT_SATISFIED 0x6985
#define SW_WRONG_DATA               0x6A80
#define SW_WRONG_LENGTH             0x6700
#define SW_CLA_NOT_SUPPORTED        0x6E00
#define SW_INS_NOT_SUPPORTED        0x6D00

static uint16_t sw_only(uint8_t *resp, uint16_t resp_max, uint16_t sw)
{
	if (resp_max < 2) {
		return 0;
	}
	resp[0] = (uint8_t)(sw >> 8);
	resp[1] = (uint8_t)(sw & 0xFF);
	return 2;
}

uint16_t u2f_handle_apdu(const uint8_t *req, uint16_t req_len,
                         uint8_t *resp, uint16_t resp_max)
{
	if (req_len < 4) {
		return sw_only(resp, resp_max, SW_WRONG_LENGTH);
	}

	uint8_t cla = req[0];
	uint8_t ins = req[1];

	if (cla != 0x00) {
		return sw_only(resp, resp_max, SW_CLA_NOT_SUPPORTED);
	}

	// Lc is either absent, one byte (short APDU), or three bytes starting with
	// a zero (extended APDU, which is what U2F over HID normally uses).
	uint16_t lc = 0;
	if (req_len >= 7 && req[4] == 0x00) {
		lc = (uint16_t)((req[5] << 8) | req[6]);
		if ((uint32_t)lc + 7 > req_len) {
			return sw_only(resp, resp_max, SW_WRONG_LENGTH);
		}
	} else if (req_len >= 5) {
		lc = req[4];
		if ((uint32_t)lc + 5 > req_len) {
			return sw_only(resp, resp_max, SW_WRONG_LENGTH);
		}
	}

	switch (ins) {
	case U2F_INS_VERSION: {
		static const char version[] = "U2F_V2";
		const uint16_t len = sizeof(version) - 1;
		if (resp_max < len + 2) {
			return 0;
		}
		for (uint16_t i = 0; i < len; i++) {
			resp[i] = (uint8_t)version[i];
		}
		resp[len] = (uint8_t)(SW_NO_ERROR >> 8);
		resp[len + 1] = (uint8_t)(SW_NO_ERROR & 0xFF);
		return (uint16_t)(len + 2);
	}

	case U2F_INS_REGISTER:
		// TODO: generate a P-256 key pair in the ATECC608A, wrap it into a key
		// handle, and sign the registration with the attestation key.  Until
		// that exists there is nothing honest to hand back, so the request is
		// refused outright rather than answered with a key that cannot sign.
		return sw_only(resp, resp_max, SW_INS_NOT_SUPPORTED);

	case U2F_INS_AUTHENTICATE:
		// No key handle this device issued can exist yet, and "wrong data" is
		// exactly what the spec says to return for a handle we do not own.
		// Check-only requests (P1 = 0x07) get the same answer for that reason.
		if (lc < 65) {
			return sw_only(resp, resp_max, SW_WRONG_LENGTH);
		}
		return sw_only(resp, resp_max, SW_WRONG_DATA);

	default:
		return sw_only(resp, resp_max, SW_INS_NOT_SUPPORTED);
	}
}
