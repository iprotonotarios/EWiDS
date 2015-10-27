#include "sofa.h"

typedef enum {
RX_SUCCESS,
TIMEOUT,
CRCFAIL
} Return_status;

// Radio packet struct
typedef struct
{
	uint16_t src;
	uint16_t dst;
} Sofa_message;
 
Sofa_message sofa_message_tx;
Sofa_message sofa_message_rx;

void send(uint32_t type)
{
	NRF_RADIO->PACKETPTR = (uint32_t)&sofa_message_tx;
	NRF_RADIO->TXADDRESS = type; 

    	// send the packet:
    	NRF_RADIO->EVENTS_READY = 0U;
    	NRF_RADIO->TASKS_TXEN   = 1;

    	while (NRF_RADIO->EVENTS_READY == 0U)
    	{
    		// wait
    	}
    	NRF_RADIO->EVENTS_END  = 0U;
    	NRF_RADIO->TASKS_START = 1U;

    	while (NRF_RADIO->EVENTS_END == 0U)
    	{
        	// wait
    	}

    	// Disable radio
    	NRF_RADIO->EVENTS_DISABLED = 0U;
    	NRF_RADIO->TASKS_DISABLE = 1U;

    	while (NRF_RADIO->EVENTS_DISABLED == 0U)
    	{
        	// wait
    	}
}

Return_status receive(uint32_t timeout)
{

//	nrf_delay_ms(50);

	Return_status return_status;
    	uint32_t wait_time = 0;
	NRF_RADIO->PACKETPTR = (uint32_t)&sofa_message_rx;

    	NRF_RADIO->EVENTS_READY = 0U;
    	// Enable radio and wait for ready
    	NRF_RADIO->TASKS_RXEN = 1U;

    	while (NRF_RADIO->EVENTS_READY == 0U)
    	{
        	// wait
    	}
	
	wait_time = rtc_time() + timeout;
	
	NRF_RADIO->EVENTS_END = 0U;
    	// Start listening and wait for address received event
    	NRF_RADIO->TASKS_START = 1U;

    	// Wait for end of packet
    	while ((NRF_RADIO->EVENTS_END == 0U)&&(rtc_time() < wait_time))
    	{
        	// wait
    	}
	
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

    	NRF_RADIO->EVENTS_DISABLED = 0U;
    	// Disable radio
    	NRF_RADIO->TASKS_DISABLE = 1U;

    	while (NRF_RADIO->EVENTS_DISABLED == 0U)
    	{
        	// wait
    	}
	
	return return_status;
}

    

void sofa_loop(void)
{
	uint32_t wait_time;
	uint32_t strobe_count = 0;
	Return_status return_status, ret_stat;

	return_status = receive(SECOND/1024);

	switch (return_status)
	{
		case RX_SUCCESS:
			switch (NRF_RADIO->RXMATCH)
			{
				case MSG_TYPE_BEACON:
					send(MSG_TYPE_ACK);
//					printf("I got a beacon, ack sent\n\r");
					break;

				case MSG_TYPE_ACK:
//					printf("I received an ACK out of nowhere, go to sleep!\n\r");			
					break;

//				default:
//					printf("Unknown message type received %lx\n\r", NRF_RADIO->RXMATCH);
			}
			printf("%ld\n\r", strobe_count);
			break;
			
		case TIMEOUT:
//			printf("timed out, sending..\n\r");
			wait_time = rtc_time()+SOFA_PERIOD; // time added sets MAX duty cycle
			while (rtc_time() < wait_time)
			{
				strobe_count++;
				send(MSG_TYPE_BEACON);
				ret_stat = receive(SECOND/1024);
//				nrf_delay_ms(10);
//				ret_stat = 5;
				if ((ret_stat == RX_SUCCESS)&&(NRF_RADIO->RXMATCH == MSG_TYPE_ACK))
				{
//					printf("Ack received after successful beaconing\n\r");
					break;		
				}
			}			
			printf("%ld\n\r", strobe_count);
			break;
			
		case CRCFAIL:
			printf("crc failed \n\r");
			break;
			
		default:
			printf("Unknown return_status\n\r");		
	}
}
