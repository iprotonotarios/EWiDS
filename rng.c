#include "sofa.h"

static uint16_t random_value = 0;
static bool lower_byte_generated = false;

void rng_init(void)
{
	// Enable Digital error correction
	NRF_RNG->CONFIG	= RNG_CONFIG_DERCEN_Enabled;

	// shorts for disabling after new value
	NRF_RNG->SHORTS = RNG_SHORTS_VALRDY_STOP_Enabled;

	// Clear event
	NRF_RNG->EVENTS_VALRDY = 0;
	
	// Enable interrupt
	NRF_RNG->INTENSET = RNG_INTENSET_VALRDY_Enabled;
	NVIC_EnableIRQ(RNG_IRQn);

	// Generate first random number
	NRF_RNG->TASKS_START = 1;	
}

uint32_t random(void)
{
	// restart random number generator to get next 8 bits
	NRF_RNG->TASKS_START = 1;
	lower_byte_generated = false;

	return random_value & 0x7fff;
}

void RNG_IRQHandler( void )
{
	// Clear event
	NRF_RNG->EVENTS_VALRDY = 0;	

	if (!lower_byte_generated)
	{
		lower_byte_generated = true;
		random_value = (uint16_t)(NRF_RNG->VALUE);
	
		// restart random number generator to get second byte
		NRF_RNG->TASKS_START = 1;
	}

	random_value = (random_value << 8) | (uint16_t)(NRF_RNG->VALUE);
}

