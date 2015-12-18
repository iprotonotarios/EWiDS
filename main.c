#include "sofa.h"
#include "average.h"

int32_t supercapSample( void )
{
	int32_t retval = 0;
	
	// Stop adc
	NRF_ADC->TASKS_STOP = 1;
	NRF_ADC->ENABLE = (ADC_ENABLE_ENABLE_Disabled << ADC_ENABLE_ENABLE_Pos);
	NRF_ADC->INTENCLR = (ADC_INTENCLR_END_Clear << ADC_INTENCLR_END_Pos);
	NRF_ADC->EVENTS_END = 0;

	// Enable and start the ADC */
	NRF_ADC->ENABLE = (ADC_ENABLE_ENABLE_Enabled << ADC_ENABLE_ENABLE_Pos);
	NRF_ADC->EVENTS_END = 0;
	NRF_ADC->TASKS_START = 1;

	/* wait till ADC is done */
	while (0 == NRF_ADC->EVENTS_END);
	NRF_ADC->EVENTS_END = 0;

	/* get result */
	retval = (NRF_ADC->RESULT & ADC_RESULT_RESULT_Msk);

	/* convert result to milivolts*/
	retval *= 6006;
	retval = ( retval >> 10UL );

	/* stop the ADC task and Disable the ADC */
	NRF_ADC->TASKS_STOP = 1;
	NRF_ADC->ENABLE = (ADC_ENABLE_ENABLE_Disabled << ADC_ENABLE_ENABLE_Pos);
	
	return retval;
}

int main(void)
{
	gpio_init();
	rng_init();
	clock_init();
	radio_init();
	average_init();
	
	// hold processing until supercap is sufficiently charged
	while ( supercapSample() < 1000 ) nrf_gpio_pin_set( LED_YELLOW );
	
	while (true){
#if defined(SNIFFER)
		sniffer_loop();
#elif defined(CHECKIN)
		checkin_loop();
#else
		__WFI();
		if (check_rtc_flag()){
			uint16_t rendezvous = sofa_loop(average_get());
			if (rendezvous > 0) average_add(rendezvous);
			//average_add((uint16_t)(NRF_FICR->DEVICEID[0]%65536));
			clear_rtc_flag();
		}
#endif		
	}
	
}


