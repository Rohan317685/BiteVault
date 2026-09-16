#include "led.h"
#include "board.h"
#include "timebase.h"

// Each pattern is a 16-slot bitmap walked left to right, one slot every
// step_ms.  A set bit means the LED is lit for that slot.
typedef struct {
	uint16_t slots;
	uint16_t step_ms;
} pattern_def_t;

static const pattern_def_t k_patterns[] = {
	[LED_OFF]   = {0x0000, 100},
	[LED_IDLE]  = {0x0001, 250}, // ~250 ms on every 4 s
	[LED_READY] = {0xFFFF, 100},
	[LED_BUSY]  = {0x5555,  50},
	[LED_TOUCH] = {0x00FF,  80}, // 640 ms on, 640 ms off
	[LED_ERROR] = {0x0F0F,  60},
};

static led_pattern_t g_base = LED_OFF;
static led_pattern_t g_active = LED_OFF;
static uint32_t g_step_at;
static uint32_t g_override_until;
static uint8_t g_slot;

static void set_output(int lit)
{
	funDigitalWrite(PIN_LED, lit ? LED_ON_LEVEL : !LED_ON_LEVEL);
}

void led_init(void)
{
	funPinMode(PIN_LED, GPIO_CFGLR_OUT_10Mhz_PP);
	set_output(0);
	g_step_at = millis();
	g_slot = 0;
}

void led_set(led_pattern_t pattern)
{
	g_base = pattern;
	if (!g_override_until && pattern != g_active) {
		g_active = pattern;
		g_slot = 0;
		g_step_at = millis();
	}
}

void led_flash(led_pattern_t pattern, uint16_t ms)
{
	g_active = pattern;
	g_slot = 0;
	g_step_at = millis();
	g_override_until = millis() + ms;
	if (!g_override_until) {
		g_override_until = 1; // 0 means "no override"; skip that value on wrap
	}
}

void led_task(void)
{
	uint32_t now = millis();

	if (g_override_until && (int32_t)(now - g_override_until) >= 0) {
		g_override_until = 0;
		g_active = g_base;
		g_slot = 0;
		g_step_at = now;
	}

	const pattern_def_t *p = &k_patterns[g_active];
	while ((uint32_t)(now - g_step_at) >= p->step_ms) {
		g_step_at += p->step_ms;
		g_slot = (g_slot + 1) & 15;
	}

	set_output((p->slots >> g_slot) & 1);
}
