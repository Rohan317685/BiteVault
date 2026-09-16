#include "i2c.h"
#include "board.h"
#include "timebase.h"

#define I2C_TIMEOUT_MS 10

static uint32_t pclk1_hz(void)
{
	// APB1 prescaler lives in CFGR0[10:8]; the top bit selects "divided".
	uint32_t ppre1 = (RCC->CFGR0 >> 8) & 0x7;
	uint32_t shift = (ppre1 & 0x4) ? ((ppre1 & 0x3) + 1) : 0;
	return FUNCONF_SYSTEM_CORE_CLOCK >> shift;
}

// Returns I2C_OK once (STAR1 & mask) matches `want`, or an error on timeout or
// on a NACK/bus fault raised while waiting.
static int wait_star1(uint16_t mask, int want)
{
	uint32_t start = millis();
	for (;;) {
		uint16_t s1 = I2C1->STAR1;
		if (!!(s1 & mask) == !!want) {
			return I2C_OK;
		}
		if (s1 & I2C_STAR1_AF) {
			I2C1->STAR1 = (uint16_t)~I2C_STAR1_AF;
			I2C1->CTLR1 |= I2C_CTLR1_STOP;
			return I2C_ERR_NACK;
		}
		if (s1 & (I2C_STAR1_BERR | I2C_STAR1_ARLO)) {
			I2C1->STAR1 = (uint16_t)~(I2C_STAR1_BERR | I2C_STAR1_ARLO);
			I2C1->CTLR1 |= I2C_CTLR1_STOP;
			return I2C_ERR_BUS;
		}
		if ((uint32_t)(millis() - start) > I2C_TIMEOUT_MS) {
			I2C1->CTLR1 |= I2C_CTLR1_STOP;
			return I2C_ERR_TIMEO;
		}
	}
}

static int wait_not_busy(void)
{
	uint32_t start = millis();
	while (I2C1->STAR2 & I2C_STAR2_BUSY) {
		if ((uint32_t)(millis() - start) > I2C_TIMEOUT_MS) {
			return I2C_ERR_TIMEO;
		}
	}
	return I2C_OK;
}

static void clear_addr_flag(void)
{
	(void)I2C1->STAR1;
	(void)I2C1->STAR2;
}

// START + address byte.  `read` picks the direction bit.
static int start_and_address(uint8_t addr7, int read)
{
	int rc = wait_not_busy();
	if (rc != I2C_OK) {
		return rc;
	}

	I2C1->CTLR1 |= I2C_CTLR1_START;
	rc = wait_star1(I2C_STAR1_SB, 1);
	if (rc != I2C_OK) {
		return rc;
	}

	I2C1->DATAR = (uint16_t)((addr7 << 1) | (read ? 1 : 0));
	return wait_star1(I2C_STAR1_ADDR, 1);
}

void i2c_init(uint32_t speed_hz)
{
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO;
	RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

	funPinMode(PIN_SCL, GPIO_CFGLR_OUT_50Mhz_AF_OD);
	funPinMode(PIN_SDA, GPIO_CFGLR_OUT_50Mhz_AF_OD);

	I2C1->CTLR1 |= I2C_CTLR1_SWRST;
	I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_SWRST;

	uint32_t pclk1 = pclk1_hz();
	uint32_t freq_mhz = pclk1 / 1000000u;
	I2C1->CTLR2 = (uint16_t)freq_mhz;

	if (speed_hz <= 100000u) {
		// Standard mode: CCR counts half a bit period in PCLK1 ticks.
		uint32_t ccr = pclk1 / (speed_hz * 2u);
		if (ccr < 4) {
			ccr = 4;
		}
		I2C1->CKCFGR = (uint16_t)ccr;
		I2C1->RTR = (uint16_t)(freq_mhz + 1); // 1000 ns max rise time
	} else {
		// Fast mode, 2:1 duty: CCR counts a third of a bit period.
		uint32_t ccr = pclk1 / (speed_hz * 3u);
		if (ccr < 1) {
			ccr = 1;
		}
		I2C1->CKCFGR = (uint16_t)(I2C_CKCFGR_FS | ccr);
		I2C1->RTR = (uint16_t)((freq_mhz * 300u) / 1000u + 1); // 300 ns max rise time
	}

	I2C1->CTLR1 |= I2C_CTLR1_PE | I2C_CTLR1_ACK;
}

int i2c_write(uint8_t addr7, const uint8_t *data, uint16_t len, int send_stop)
{
	int rc = start_and_address(addr7, 0);
	if (rc != I2C_OK) {
		return rc;
	}
	clear_addr_flag();

	for (uint16_t i = 0; i < len; i++) {
		rc = wait_star1(I2C_STAR1_TXE, 1);
		if (rc != I2C_OK) {
			return rc;
		}
		I2C1->DATAR = data[i];
	}

	rc = wait_star1(I2C_STAR1_BTF, 1);
	if (rc != I2C_OK) {
		return rc;
	}

	if (send_stop) {
		I2C1->CTLR1 |= I2C_CTLR1_STOP;
	}
	return I2C_OK;
}

int i2c_read(uint8_t addr7, uint8_t *data, uint16_t len)
{
	int rc;

	if (len == 0) {
		return I2C_OK;
	}

	if (len == 1) {
		// Single byte: ACK has to be cleared *before* the address flag, and the
		// STOP armed immediately after, or the device clocks out a second byte.
		I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_ACK;
		rc = start_and_address(addr7, 1);
		if (rc != I2C_OK) {
			I2C1->CTLR1 |= I2C_CTLR1_ACK;
			return rc;
		}
		clear_addr_flag();
		I2C1->CTLR1 |= I2C_CTLR1_STOP;

		rc = wait_star1(I2C_STAR1_RXNE, 1);
		if (rc == I2C_OK) {
			data[0] = (uint8_t)I2C1->DATAR;
		}
		I2C1->CTLR1 |= I2C_CTLR1_ACK;
		return rc;
	}

	if (len == 2) {
		// Two bytes: POS makes the NACK apply to the second byte, and both are
		// lifted out of the shift register together once BTF is up.
		I2C1->CTLR1 |= I2C_CTLR1_ACK | I2C_CTLR1_POS;
		rc = start_and_address(addr7, 1);
		if (rc != I2C_OK) {
			I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_POS;
			return rc;
		}
		clear_addr_flag();
		I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_ACK;

		rc = wait_star1(I2C_STAR1_BTF, 1);
		if (rc != I2C_OK) {
			I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_POS;
			I2C1->CTLR1 |= I2C_CTLR1_ACK;
			return rc;
		}
		I2C1->CTLR1 |= I2C_CTLR1_STOP;
		data[0] = (uint8_t)I2C1->DATAR;
		data[1] = (uint8_t)I2C1->DATAR;

		I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_POS;
		I2C1->CTLR1 |= I2C_CTLR1_ACK;
		return I2C_OK;
	}

	I2C1->CTLR1 |= I2C_CTLR1_ACK;
	rc = start_and_address(addr7, 1);
	if (rc != I2C_OK) {
		return rc;
	}
	clear_addr_flag();

	uint16_t i = 0;
	while (len - i > 3) {
		rc = wait_star1(I2C_STAR1_RXNE, 1);
		if (rc != I2C_OK) {
			return rc;
		}
		data[i++] = (uint8_t)I2C1->DATAR;
	}

	// Last three bytes: with N-3 read and BTF up, the shift register holds
	// N-2 and N-1, so the NACK and STOP must be set up now.
	rc = wait_star1(I2C_STAR1_BTF, 1);
	if (rc != I2C_OK) {
		return rc;
	}
	I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_ACK;
	data[i++] = (uint8_t)I2C1->DATAR;
	I2C1->CTLR1 |= I2C_CTLR1_STOP;
	data[i++] = (uint8_t)I2C1->DATAR;

	rc = wait_star1(I2C_STAR1_RXNE, 1);
	if (rc != I2C_OK) {
		I2C1->CTLR1 |= I2C_CTLR1_ACK;
		return rc;
	}
	data[i++] = (uint8_t)I2C1->DATAR;

	I2C1->CTLR1 |= I2C_CTLR1_ACK;
	return I2C_OK;
}

int i2c_probe(uint8_t addr7)
{
	int rc = start_and_address(addr7, 0);
	if (rc == I2C_OK) {
		clear_addr_flag();
		I2C1->CTLR1 |= I2C_CTLR1_STOP;
	}
	return rc;
}

void i2c_wake_pulse(uint16_t low_us)
{
	// The peripheral owns the pins, so it has to let go before SDA can be
	// parked low for longer than a bit time.
	I2C1->CTLR1 &= (uint16_t)~I2C_CTLR1_PE;

	funPinMode(PIN_SDA, GPIO_CFGLR_OUT_10Mhz_OD);
	funDigitalWrite(PIN_SDA, 0);
	Delay_Us(low_us);
	funDigitalWrite(PIN_SDA, 1);
	funPinMode(PIN_SDA, GPIO_CFGLR_OUT_50Mhz_AF_OD);

	I2C1->CTLR1 |= I2C_CTLR1_PE | I2C_CTLR1_ACK;
}
