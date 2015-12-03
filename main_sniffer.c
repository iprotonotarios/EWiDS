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
		sniffer_loop();
	}
	
}


