#include "sofa.h"
#include "average.h"

int main(void)
{
	gpio_init();
	rng_init();
	clock_init();
	radio_init();
	average_init();
	
	while (true){
#if defined(SNIFFER)
		sniffer_loop();
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


