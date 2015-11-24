#include "sofa.h"

#define MYID (uint16_t)(NRF_FICR->DEVICEID[0]%65536)
 
typedef enum {
RX_SUCCESS,
TIMEOUT,
CRCFAIL,
WRONG_ADDR
} Return_status;

// Radio packet struct
typedef struct
{
	uint16_t src;
	uint16_t dst;
	uint16_t rendezvous;
} Sofa_message;
 
Sofa_message sofa_message_tx;
Sofa_message sofa_message_rx;
uint16_t measured_rendezvous, averaged_rendezvous;


void send(uint32_t _type, uint16_t _dst)
{
       nrf_gpio_pin_set(GPIO_1);
	// prepare packet	
	sofa_message_tx.src = MYID;
	sofa_message_tx.dst = _dst;
	sofa_message_tx.rendezvous = averaged_rendezvous;
	NRF_RADIO->PACKETPTR = (uint32_t)&sofa_message_tx;
	NRF_RADIO->TXADDRESS = _type; 
    	// switch to rx
    	NRF_RADIO->EVENTS_READY = 0U;
    	NRF_RADIO->TASKS_TXEN   = 1;
    	while (NRF_RADIO->EVENTS_READY == 0U);
    	NRF_RADIO->EVENTS_END  = 0U;
	// send the packet
    	NRF_RADIO->TASKS_START = 1U;
    	while (NRF_RADIO->EVENTS_END == 0U);
    	// Disable radio
    	NRF_RADIO->EVENTS_DISABLED = 0U;
    	NRF_RADIO->TASKS_DISABLE = 1U;
    	while (NRF_RADIO->EVENTS_DISABLED == 0U);
       nrf_gpio_pin_clear(GPIO_1);
}

Return_status receive(uint32_t timeout)
{
       nrf_gpio_pin_set(GPIO_2);
	Return_status return_status;
    	uint32_t wait_time = 0;
	NRF_RADIO->PACKETPTR = (uint32_t)&sofa_message_rx;
    	NRF_RADIO->EVENTS_READY = 0U;
    	// Enable radio and wait for ready
    	NRF_RADIO->TASKS_RXEN = 1U;
    	while (NRF_RADIO->EVENTS_READY == 0U);
	// set listening timeout
	wait_time = rtc_time() + timeout;	
	NRF_RADIO->EVENTS_END = 0U;
    	// Start listening and wait for address received event
    	NRF_RADIO->TASKS_START = 1U;
    	// Wait for end of packet
    	while ((NRF_RADIO->EVENTS_END == 0U)&&(rtc_time() < wait_time));
	if (NRF_RADIO->EVENTS_END == 0U)
	{
		return_status =  TIMEOUT;
	} else {
		if (NRF_RADIO->CRCSTATUS == 1U) 
    		{
			return_status = RX_SUCCESS;
	    	} else {
			return_status =  CRCFAIL;
		}
	}
	if ((sofa_message_rx.dst != 0) && (sofa_message_rx.dst != MYID)){
		return_status =  WRONG_ADDR;
	}
    	NRF_RADIO->EVENTS_DISABLED = 0U;
    	// Disable radio
    	NRF_RADIO->TASKS_DISABLE = 1U;
    	while (NRF_RADIO->EVENTS_DISABLED == 0U);
       nrf_gpio_pin_clear(GPIO_2);	
	return return_status;
}

    

uint16_t sofa_loop(uint16_t _rendezvous)
{
	uint64_t wait_time,time_strobe_start;
	uint32_t strobe_count = 0;
	Return_status return_status, ret_stat;
	
	averaged_rendezvous = _rendezvous;
	measured_rendezvous = 0;
	
	// Listening backoff
	return_status = receive(SECOND/1024);
	switch (return_status)
	{
		case RX_SUCCESS:
			switch (NRF_RADIO->RXMATCH)
			{
				case MSG_TYPE_BEACON:
					send(MSG_TYPE_ACK,sofa_message_rx.src);
					//printf("I got a beacon, ack sent\n\r");
					break;

				case MSG_TYPE_ACK:
					//printf("I received an ACK out of nowhere, go to sleep!\n\r");			
					break;

				default:
					printf("UNKNOWN MSG TYPE %lx\n", NRF_RADIO->RXMATCH);
			}
			//printf("%ld\n\r", strobe_count);
			break;
			
		case TIMEOUT:
			//printf("timed out, sending..\n\r");
			time_strobe_start = rtc_time();
			wait_time = rtc_time()+(SOFA_PERIOD*2); // time added sets MAX duty cycle
			while (rtc_time() < wait_time)
			{
				strobe_count++;
				send(MSG_TYPE_BEACON,0);
				ret_stat = receive(SECOND/2048);
				if ((ret_stat == RX_SUCCESS)&&(NRF_RADIO->RXMATCH == MSG_TYPE_ACK))
				{
					//check if the rendezvous fits 16 bits (less than 2 seconds)
					if ((rtc_time()-time_strobe_start) < 65536){
						measured_rendezvous = (uint16_t)(rtc_time()-time_strobe_start);
					}
					//printf("Ack received after successful beaconing\n\r");
					send(MSG_TYPE_SELECT,sofa_message_rx.src);
					break;		
				}
			}			
			//printf("%ld\n\r", strobe_count);
			break;
		
		case WRONG_ADDR:
			printf("WRONG ADDR\n");
			break;
			
		case CRCFAIL:
			printf("CRC FAIL\n");
			break;
			
		default:
			printf("UNKNOWN RX RETURN\n");		
	}
	return measured_rendezvous;
}

void sniffer_loop(void)
{
	Return_status return_status;
	
	//Listen for the next packet
	return_status = receive(SECOND*2);
	switch (return_status)
	{
		// we are in sniffer mode. so we consider also messages for other destinations
		case WRONG_ADDR:
		case RX_SUCCESS:
			switch (NRF_RADIO->RXMATCH)
			{
				case MSG_TYPE_BEACON:
					break;

				case MSG_TYPE_ACK:
					//printf("A %d %d %d\n",sofa_message_rx.src,sofa_message_rx.dst,sofa_message_rx.rendezvous);			
					break;
				case MSG_TYPE_SELECT:
					printf("\r000001.NA.8>[%03x]%03lx.1(2.03.%04x|5.06.0007|8.09.0010)+11111111\n",(sofa_message_rx.src)%4096,(SECOND/(sofa_message_rx.rendezvous)),(sofa_message_rx.dst)%4096);
					//printf("S %d %d %d\n",sofa_message_rx.src,sofa_message_rx.dst,sofa_message_rx.rendezvous);	
					break;

				default:
					printf("UM: %lx\n", NRF_RADIO->RXMATCH);
			}
			break;
			
		case TIMEOUT:
			printf("TIMEOUT\n");
			break;
			
		case CRCFAIL:
			printf("CRC\n");
			break;
			
		default:
			printf("US: %d\n",return_status);		
	}
}

