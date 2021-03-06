#include "sofa.h"

static uint64_t rtc_counter = 0;
static bool rtc_flag = false;

void clock_init(void)
{
    	/* Start 16 MHz crystal oscillator */
    	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    	NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    	/* Wait for the external oscillator to start up */
    	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    	{
    		// Do nothing.
    	}

	/* Start low frequency crystal oscillator */
    	NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    	NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    	{
    		// Do nothing.
    	}

	NRF_RTC1->PRESCALER 	= 0;
	NRF_RTC1->INTENSET	= RTC_INTENSET_COMPARE0_Msk;
	NRF_RTC1->CC[0]		= 16384+random();
	NRF_RTC1->TASKS_START	= 1;
	
	NVIC_EnableIRQ(RTC1_IRQn);
}


void RTC1_IRQHandler(void)
{
	if (NRF_RTC1->EVENTS_COMPARE[0])
    	{
		rtc_counter += (uint64_t)(NRF_RTC1->COUNTER);
		NRF_RTC1->TASKS_CLEAR = 1;
		while(NRF_RTC1->COUNTER!=0);
		NRF_RTC1->EVENTS_COMPARE[0] = 0;
		NRF_RTC1->CC[0]	= 16384+random();
		rtc_flag = true;
    	}
}

uint64_t rtc_time(void)
{
	return rtc_counter+(uint64_t)(NRF_RTC1->COUNTER);
}

uint64_t short_rtc_time(void)
{
	return (uint64_t)(NRF_RTC1->COUNTER);
}

bool check_rtc_flag(void)
{
	return rtc_flag;
}

void clear_rtc_flag(void)
{
	rtc_flag = false;
}

