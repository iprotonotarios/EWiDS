#include "sofa.h"

int main(void)
{
	gpio_init();
	rng_init();
	clock_init();
	radio_init();

	while (true)
	{
		__WFI();
		if (check_rtc_flag())
		{
			sofa_loop();
			clear_rtc_flag();
		}
	}
}


