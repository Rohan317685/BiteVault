#include "button.h"
#include "board.h"
#include "timebase.h"

#define DEBOUNCE_MS 20

static uint8_t g_stable;      // debounced level: 1 = down
static uint8_t g_candidate;   // level we are currently timing
static uint32_t g_changed_at;
static uint32_t g_pressed_at; // when the last unconsumed press landed
static uint8_t g_press_pending;

static int raw_down(void)
{
	return funDigitalRead(PIN_BUTTON) == BUTTON_DOWN_LEVEL;
}

void button_init(void)
{
	// Input with pull-up/pull-down selected by the output data register; a 1
	// there picks pull-up, which is what SW1 needs (it shorts the pin to GND).
	funPinMode(PIN_BUTTON, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(PIN_BUTTON, 1);

	g_stable = g_candidate = raw_down();
	g_changed_at = millis();
	g_press_pending = 0;
}

void button_task(void)
{
	int now_level = raw_down();
	uint32_t now = millis();

	if (now_level != g_candidate) {
		g_candidate = now_level;
		g_changed_at = now;
		return;
	}

	if (g_candidate != g_stable && (uint32_t)(now - g_changed_at) >= DEBOUNCE_MS) {
		g_stable = g_candidate;
		if (g_stable) {
			g_pressed_at = now;
			g_press_pending = 1;
		}
	}
}

int button_is_down(void)
{
	return g_stable;
}

int button_take_press(uint32_t max_age_ms)
{
	if (!g_press_pending) {
		return 0;
	}
	if ((uint32_t)(millis() - g_pressed_at) > max_age_ms) {
		g_press_pending = 0; // too old to count for anything
		return 0;
	}
	g_press_pending = 0;
	return 1;
}
