#include "timebase.h"
#include "ch32fun.h"

static volatile uint32_t g_millis;

void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
	// Advance the compare register rather than reloading it, so the tick does
	// not drift with interrupt latency.
	SysTick->CMP += DELAY_MS_TIME;
	SysTick->SR = 0;
	g_millis++;
}

void timebase_init(void)
{
	SysTick->CTLR = 0;
	SysTick->CMP = DELAY_MS_TIME - 1;
	SysTick->CNT = 0;
	g_millis = 0;

	// STCLK = HCLK/1, counter + interrupt on.  Deliberately not setting STRE so
	// ch32fun's own busy-wait Delay_Us()/Delay_Ms() keep working.
	SysTick->CTLR |= SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
	NVIC_EnableIRQ(SysTick_IRQn);
}

uint32_t millis(void)
{
	return g_millis;
}
