#include <inttypes.h>
#include <stdio.h>
#include "wristband.h"
#include "nrf_drv_gpiote.h"
#include "app_uart.h"
#include "nrf_delay.h"

#define SOFA_PERIOD 32768UL	// one second
#define SECOND 32768UL		// one second

#define PREFIX0NOTUSED 	0x0UL	// Not used
#define MSG_TYPE_BEACON	0x1UL	// Beacon
#define MSG_TYPE_ACK 	0x2UL	// Ack
#define MSG_TYPE_SELECT	0x3UL	// Select packet

// GPIO
void gpio_init(void);

// RTC - clocks
void clock_init(void);
uint64_t rtc_time(void);
uint64_t short_rtc_time(void);
bool check_rtc_flag(void);
void clear_rtc_flag(void);

// Radio
void radio_init(void);
void send_sniffer(uint16_t src,uint16_t dst);
		
// Sofa mac
uint16_t sofa_loop(uint16_t);

// Sofa sniffer
void sniffer_loop(void);

// Random number generator
void rng_init(void);
uint32_t random(void);
