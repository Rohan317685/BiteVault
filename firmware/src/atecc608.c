#include "atecc608.h"
#include "i2c.h"
#include "timebase.h"
#include "ch32fun.h"

// Word addresses (the first byte of every write to the device).
#define WORD_RESET 0x00
#define WORD_SLEEP 0x01
#define WORD_IDLE  0x02
#define WORD_CMD   0x03

// Opcodes.
#define OP_READ   0x02
#define OP_RANDOM 0x1B
#define OP_INFO   0x30

// Worst-case execution times from the ATECC608A datasheet, in ms.
#define EXEC_READ   5
#define EXEC_RANDOM 23
#define EXEC_INFO   5

// Wake timing: tWLO (SDA held low) is 60 us minimum, tWHI (bus quiet after the
// pulse) is 1500 us minimum.  Both get a little headroom here.
#define WAKE_LOW_US  80
#define WAKE_HIGH_US 1800

#define STATUS_SUCCESS   0x00
#define STATUS_AFTER_WAKE 0x11

static uint8_t g_last_status;

// The CRC-16 the device uses: polynomial 0x8005, LSB-first over the packet from
// the count byte up to (but not including) the CRC itself.
static void atecc_crc16(const uint8_t *data, uint8_t len, uint8_t out[2])
{
	uint16_t crc = 0;

	for (uint8_t i = 0; i < len; i++) {
		for (uint8_t bit = 0x01; bit; bit <<= 1) {
			uint8_t data_bit = (data[i] & bit) ? 1 : 0;
			uint8_t crc_bit = (uint8_t)(crc >> 15);
			crc <<= 1;
			if (data_bit != crc_bit) {
				crc ^= 0x8005;
			}
		}
	}

	out[0] = (uint8_t)(crc & 0xFF);
	out[1] = (uint8_t)(crc >> 8);
}

static void atecc_sleep(void)
{
	uint8_t word = WORD_SLEEP;
	(void)i2c_write(ATECC_I2C_ADDR, &word, 1, 1);
}

static int atecc_wake(void)
{
	uint8_t resp[4];

	i2c_wake_pulse(WAKE_LOW_US);
	Delay_Us(WAKE_HIGH_US);

	// A woken device answers with the 4-byte status packet {04 11 33 43}.
	// Give it a couple of tries: the first read can land before it is listening.
	for (int attempt = 0; attempt < 3; attempt++) {
		if (i2c_read(ATECC_I2C_ADDR, resp, sizeof(resp)) == I2C_OK) {
			uint8_t crc[2];
			atecc_crc16(resp, 2, crc);
			if (resp[0] == 4 && resp[1] == STATUS_AFTER_WAKE &&
			    resp[2] == crc[0] && resp[3] == crc[1]) {
				return ATECC_OK;
			}
		}
		Delay_Us(500);
	}

	return ATECC_ERR_WAKE;
}

// Reads one response packet.  The device restarts its output buffer on every
// read transaction, so the length byte can be fetched on its own first and the
// whole packet re-read afterwards.
static int atecc_receive(uint8_t *buf, uint8_t buf_len, uint8_t *out_len)
{
	uint8_t count;

	if (i2c_read(ATECC_I2C_ADDR, &count, 1) != I2C_OK) {
		return ATECC_ERR_COMM;
	}
	if (count < 4 || count > buf_len) {
		return ATECC_ERR_COMM;
	}
	if (i2c_read(ATECC_I2C_ADDR, buf, count) != I2C_OK) {
		return ATECC_ERR_COMM;
	}
	if (buf[0] != count) {
		return ATECC_ERR_COMM;
	}

	uint8_t crc[2];
	atecc_crc16(buf, (uint8_t)(count - 2), crc);
	if (buf[count - 2] != crc[0] || buf[count - 1] != crc[1]) {
		return ATECC_ERR_CRC;
	}

	*out_len = count;
	return ATECC_OK;
}

// Sends one command and collects its response payload.  `resp` receives
// `resp_len` bytes of command data (the count and CRC are stripped).
// The caller is responsible for having woken the device.
static int atecc_command(uint8_t opcode, uint8_t p1, uint16_t p2,
                         const uint8_t *data, uint8_t data_len,
                         uint8_t *resp, uint8_t resp_len, uint16_t exec_ms)
{
	uint8_t tx[8 + 32];
	uint8_t rx[40];

	if ((uint32_t)data_len + 8 > sizeof(tx) || (uint32_t)resp_len + 3 > sizeof(rx)) {
		return ATECC_ERR_PARAM;
	}

	tx[0] = WORD_CMD;
	tx[1] = (uint8_t)(7 + data_len); // count covers itself, the body and the CRC
	tx[2] = opcode;
	tx[3] = p1;
	tx[4] = (uint8_t)(p2 & 0xFF);
	tx[5] = (uint8_t)(p2 >> 8);
	for (uint8_t i = 0; i < data_len; i++) {
		tx[6 + i] = data[i];
	}
	atecc_crc16(&tx[1], (uint8_t)(5 + data_len), &tx[6 + data_len]);

	if (i2c_write(ATECC_I2C_ADDR, tx, (uint16_t)(8 + data_len), 1) != I2C_OK) {
		return ATECC_ERR_COMM;
	}

	// The device NACKs its address while it is still computing, so poll from
	// the datasheet's worst case until roughly double that.
	uint32_t deadline = millis() + exec_ms * 2u + 5u;
	Delay_Us(exec_ms * 1000u);

	for (;;) {
		uint8_t packet_len = 0;
		int rc = atecc_receive(rx, sizeof(rx), &packet_len);

		if (rc == ATECC_OK) {
			if (packet_len == 4 && resp_len != 1) {
				g_last_status = rx[1];
				return ATECC_ERR_STATUS;
			}
			if (packet_len != resp_len + 3) {
				return ATECC_ERR_COMM;
			}
			for (uint8_t i = 0; i < resp_len; i++) {
				resp[i] = rx[1 + i];
			}
			return ATECC_OK;
		}

		if (rc != ATECC_ERR_COMM || (int32_t)(millis() - deadline) >= 0) {
			return rc;
		}
		Delay_Us(500);
	}
}

uint8_t atecc_last_status(void)
{
	return g_last_status;
}

int atecc_probe(uint8_t revision[4])
{
	uint8_t rev[4];

	int rc = atecc_wake();
	if (rc != ATECC_OK) {
		return rc;
	}

	rc = atecc_command(OP_INFO, 0x00 /* Revision */, 0x0000, 0, 0,
	                   rev, sizeof(rev), EXEC_INFO);
	atecc_sleep();

	if (rc == ATECC_OK && revision) {
		for (int i = 0; i < 4; i++) {
			revision[i] = rev[i];
		}
	}
	return rc;
}

int atecc_random(uint8_t out[32])
{
	int rc = atecc_wake();
	if (rc != ATECC_OK) {
		return rc;
	}

	rc = atecc_command(OP_RANDOM, 0x00 /* update seed */, 0x0000, 0, 0,
	                   out, 32, EXEC_RANDOM);
	atecc_sleep();
	return rc;
}

int atecc_serial(uint8_t out[9])
{
	uint8_t block[32];

	int rc = atecc_wake();
	if (rc != ATECC_OK) {
		return rc;
	}

	// Config zone (0x00), 32-byte read (0x80), block 0, offset 0.
	rc = atecc_command(OP_READ, 0x80, 0x0000, 0, 0, block, sizeof(block), EXEC_READ);
	atecc_sleep();

	if (rc != ATECC_OK) {
		return rc;
	}

	// SN[0:3] live at bytes 0-3 and SN[4:8] at bytes 8-12; bytes 4-7 are the
	// revision, which is why the serial is split.
	for (int i = 0; i < 4; i++) {
		out[i] = block[i];
	}
	for (int i = 0; i < 5; i++) {
		out[4 + i] = block[8 + i];
	}
	return ATECC_OK;
}
