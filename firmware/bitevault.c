// BiteVault - basic firmware for the CH32V203 security key.
//
// What this does today:
//   * brings the board up (96 MHz off Y1, USB, I2C to the secure element)
//   * enumerates as a FIDO HID device and speaks the CTAPHID transport
//   * answers U2F_VERSION; register/authenticate are not implemented yet
//   * bring-up self test: talks to the ATECC608A at boot and on every press
//     of SW1, and reports the result on D1 and over the debug channel
//
// See firmware/README.md for the state of play and what comes next.

#include "ch32fun.h"
#include <stdio.h>

#include "fsusb.h"

#include "board.h"
#include "timebase.h"
#include "led.h"
#include "button.h"
#include "i2c.h"
#include "atecc608.h"
#include "ctaphid.h"

#define I2C_SPEED_HZ 100000 // conservative for the 4.7k pull-ups on R3/R4

static int g_secure_element_ok;

static void print_hex(const char *label, const uint8_t *data, int len)
{
	printf("%s", label);
	for (int i = 0; i < len; i++) {
		printf(" %02x", data[i]);
	}
	printf("\n");
}

// Wakes the ATECC608A, reads its revision and serial, and pulls one block of
// random data out of it.  This is the check worth running on a freshly
// assembled board: it exercises the I2C routing, the pull-ups and the part.
static int secure_element_selftest(void)
{
	uint8_t revision[4];
	uint8_t serial[9];
	uint8_t random[32];

	int rc = atecc_probe(revision);
	if (rc != ATECC_OK) {
		printf("atecc: probe failed (%d, status %02x)\n", rc, atecc_last_status());
		return 0;
	}
	print_hex("atecc: revision", revision, sizeof(revision));

	rc = atecc_serial(serial);
	if (rc == ATECC_OK) {
		print_hex("atecc: serial  ", serial, sizeof(serial));
	} else {
		printf("atecc: serial read failed (%d)\n", rc);
	}

	rc = atecc_random(random);
	if (rc == ATECC_OK) {
		// A device whose config zone is still unlocked answers with a fixed
		// 0xFFFF0000... pattern instead of entropy.  That is expected on a
		// virgin part, not a fault.
		print_hex("atecc: random  ", random, 8);
	} else {
		printf("atecc: random failed (%d, status %02x)\n", rc, atecc_last_status());
	}

	return 1;
}

// ---------------------------------------------------------------------------
// ch32fun USBFS callbacks.  These run in the USB interrupt, so they only hand
// data over to the CTAPHID layer and return.
// ---------------------------------------------------------------------------

void HandleDataOut(struct _USBState *ctx, int endp, uint8_t *data, int len)
{
	(void)ctx;
	if (endp == 1) {
		ctaphid_rx_packet(data, len);
	}
}

int HandleInRequest(struct _USBState *ctx, int endp, uint8_t *data, int len)
{
	(void)ctx; (void)endp; (void)data; (void)len;
	return 0;
}

int HandleSetupCustom(struct _USBState *ctx, int setup_code)
{
	(void)ctx; (void)setup_code;
	return 0;
}

int HandleHidUserGetReportSetup(struct _USBState *ctx, tusb_control_request_t *req)
{
	(void)ctx; (void)req;
	return 0;
}

int HandleHidUserSetReportSetup(struct _USBState *ctx, tusb_control_request_t *req)
{
	(void)ctx; (void)req;
	return 0;
}

// Some hosts push CTAPHID reports through SET_REPORT on the control pipe
// instead of the interrupt endpoint; both land in the same queue.
void HandleHidUserReportDataOut(struct _USBState *ctx, uint8_t *data, int len)
{
	(void)ctx;
	ctaphid_rx_packet(data, len);
}

int HandleHidUserReportDataIn(struct _USBState *ctx, uint8_t *data, int len)
{
	(void)ctx; (void)data;
	return len;
}

void HandleHidUserReportOutComplete(struct _USBState *ctx)
{
	(void)ctx;
}

int main(void)
{
	SystemInit();
	funGpioInitAll();

	timebase_init();
	led_init();
	button_init();
	led_set(LED_IDLE);

	printf("\nBiteVault starting (core %d Hz)\n", (int)FUNCONF_SYSTEM_CORE_CLOCK);

	i2c_init(I2C_SPEED_HZ);
	g_secure_element_ok = secure_element_selftest();
	if (!g_secure_element_ok) {
		led_set(LED_ERROR);
	}

	ctaphid_init();
	USBFSSetup();
	printf("usb: up\n");

	int was_configured = 0;

	for (;;) {
		led_task();
		button_task();
		ctaphid_task();

		int configured = USBFSCTX.USBFS_DevEnumStatus != 0;
		if (configured != was_configured) {
			was_configured = configured;
			printf("usb: %s\n", configured ? "configured" : "disconnected");
		}

		if (g_secure_element_ok) {
			if (ctaphid_is_busy()) {
				led_set(LED_BUSY);
			} else {
				led_set(configured ? LED_READY : LED_IDLE);
			}
		}

		// SW1 is the user presence button.  Nothing needs presence yet, so for
		// now a press re-runs the secure element check - a quick way to tell a
		// good board from a bad one without a debugger attached.
		if (button_take_press(2000) && !ctaphid_is_busy()) {
			printf("button: running secure element self test\n");
			int ok = secure_element_selftest();
			g_secure_element_ok = ok;
			led_flash(ok ? LED_TOUCH : LED_ERROR, 600);
		}
	}
}
