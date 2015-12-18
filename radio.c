#include "sofa.h"

/* These are set to zero to reduce overal packet size */
#define PACKET_S1_FIELD_SIZE             (0UL)  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             (0UL)  /**< Packet S0 field size in bits. */
#define PACKET_LENGTH_FIELD_SIZE         (0UL)  /**< Packet length field size in bits. */

#define PACKET_BASE_ADDRESS_LENGTH       (4UL)  //!< Packet base address length field size in bytes
#define PACKET_STATIC_LENGTH             (6UL)  //!< Packet static length in bytes
#define PACKET_PAYLOAD_MAXSIZE           (PACKET_STATIC_LENGTH)  //!< Packet payload maximum size in bytes

static uint8_t tx_power = RADIO_TXPOWER_TXPOWER_0dBm;
static uint8_t tx_channel = 75;


struct
{
	uint32_t  density   : 10; 	// 0...1023
	uint32_t  protocol  :  1; 	// 0 = fusion, 1 = sofa
	uint32_t  shortId   : 11; 	// id & 0x7ff; wristband = 1 - 2047 : 0 = anchor

	struct
	{
		uint32_t  rxOffset  : 4;  	// time in seconds / cycles since RX (0-31)
		uint32_t  rssi      : 6;  	// = -1 * (actual signal strength + 30)
		uint32_t  anchor    : 14; 	// = anchor id & 0x3FFF (1-16383, 0 is invalid)
	}
	__attribute__ ((packed)) 
	anchor[ 3 ];
	
	uint32_t validation;
} __attribute__ ((packed)) snifferData;

//struct snifferPkt snifferData;

// void radio_init_ewids()
// {
// 	/* clear events */
// // 	NRF_RADIO->EVENTS_END	= 0;
// // 	NRF_RADIO->EVENTS_ADDRESS	= 0;
// // 	NRF_RADIO->EVENTS_RSSIEND	= 0;
// // 	NRF_RADIO->EVENTS_BCMATCH	= 0;
// // 	NRF_RADIO->EVENTS_DEVMATCH	= 0;
// // 	NRF_RADIO->EVENTS_DEVMISS	= 0;
// // 	NRF_RADIO->EVENTS_PAYLOAD	= 0;
// // 	NRF_RADIO->EVENTS_READY	= 0;
//
// 	/* set speed */
// 	NRF_RADIO->MODE = ( RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos );
// 	/* set channel */
// 	NRF_RADIO->FREQUENCY = 80;
// 	/* set logic address */
// 	NRF_RADIO->TXADDRESS = 5;
//
// 	/* set tx strength */
// 	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos4dBm;
// 	/* Packet size configuration: 16 bytes */
// 	NRF_RADIO->PCNF0 =
// 		( 0 << RADIO_PCNF0_S0LEN_Pos ) |
// 		( 0 << RADIO_PCNF0_S1LEN_Pos ) |
// 		( 0 << RADIO_PCNF0_LFLEN_Pos );
// 	NRF_RADIO->PCNF1 =
// 		( RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos ) |
// 		( RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos  ) |
// 		( 4  << RADIO_PCNF1_BALEN_Pos                             ) |
// 		( 16 << RADIO_PCNF1_STATLEN_Pos                           ) |
// 		( 16 << RADIO_PCNF1_MAXLEN_Pos                            );
// 	/* set address */
// 	NRF_RADIO->PREFIX0 =
// 		( (uint32_t)0x50        ) |  // not used
// 		( (uint32_t)0x51 <<   8 ) |  // beacon
// 		( (uint32_t)0x52 <<  16 ) |  // beacon ack (not used)
// 		( (uint32_t)0x53 <<  24 );   // sensor message (not used)
// 	NRF_RADIO->PREFIX1 =
// 		( (uint32_t)0x54        ) |  // task message
// 		( (uint32_t)0x55 <<   8 ) |  // authentication message
// 		( (uint32_t)0x56 <<  16 ) |  // direct task (not used)
// 		( (uint32_t)0x57 <<  24 );   // not used
// 	NRF_RADIO->BASE0   = 0x7E7E7E7E; // not used
// 	NRF_RADIO->BASE1   = 0xEAEAEAEA;
// 	/* CRC Config : disable CRC */
// 	NRF_RADIO->CRCCNF  = ( RADIO_CRCCNF_LEN_Disabled << RADIO_CRCCNF_LEN_Pos );
//
// 	/* reset events */
// 	// NRF_RADIO->EVENTS_ADDRESS = 0;
// 	// NRF_RADIO->EVENTS_END = 0;
// 	// NRF_RADIO->EVENTS_RSSIEND = 0;
//
// 	/* enable radio interrupt */
// 	//NRF_RADIO->INTENSET = ( RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos );
// 	//NVIC_EnableIRQ( RADIO_IRQn );
// }

void radio_init()
{

    NRF_RADIO->TXPOWER   = tx_power; 
    NRF_RADIO->FREQUENCY = tx_channel;
    NRF_RADIO->MODE      = (RADIO_MODE_MODE_Nrf_2Mbit << RADIO_MODE_MODE_Pos);

	NRF_RADIO->PREFIX0 = 
	( (uint32_t)0x50        ) |  // not used
	( (uint32_t)0x51 <<   8 ) |  // beacon
	( (uint32_t)0x52 <<  16 ) |  // beacon ack (not used)
	( (uint32_t)0x53 <<  24 );   // sensor message (not used)
	NRF_RADIO->PREFIX1 =
	( (uint32_t)0x54        ) |  // task message
	( (uint32_t)0x55 <<   8 ) |  // authentication message
	( (uint32_t)0x56 <<  16 ) |  // direct task (not used)
	( (uint32_t)0x57 <<  24 );   // not used
	NRF_RADIO->BASE0   = 0x7E7E7E7E; // not used
	NRF_RADIO->BASE1   = 0xEAEAEAEA; 


    NRF_RADIO->RXADDRESSES = 	14UL;
				/*(1 << MSG_TYPE_BEACON) 	|
				(1 << MSG_TYPE_ACK   )	|
				(1 << MSG_TYPE_SELECT);    // 00001110*/

NRF_RADIO->PCNF0 = 
	( 0 << RADIO_PCNF0_S0LEN_Pos ) |
	( 0 << RADIO_PCNF0_S1LEN_Pos ) |
	( 0 << RADIO_PCNF0_LFLEN_Pos );
NRF_RADIO->PCNF1 = 
	( RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos ) |
	( RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos  ) |
	( 4  << RADIO_PCNF1_BALEN_Pos                             ) |
	( 16 << RADIO_PCNF1_STATLEN_Pos                           ) |
	( 16 << RADIO_PCNF1_MAXLEN_Pos                            );

	NRF_RADIO->CRCCNF  = ( RADIO_CRCCNF_LEN_Disabled << RADIO_CRCCNF_LEN_Pos );

    
}

// void radio_init_WORKING()
// {
//
//     // Radio config
//     NRF_RADIO->TXPOWER   = RADIO_TXPOWER_TXPOWER_Neg20dBm; // -20 dBm
//     NRF_RADIO->FREQUENCY = 0UL;           // Frequency bin 0, 2400 Hz - before wifi
//     NRF_RADIO->MODE      = RADIO_MODE_MODE_Nrf_2Mbit;
//
//     // Radio address config
//     NRF_RADIO->PREFIX0	= 	        ((uint32_t)(0xC3) << 24) // Prefix byte of address 3 converted to nRF24L series format
//       | ((uint32_t)(0xC2) << 16) // Prefix byte of address 2 converted to nRF24L series format
//       | ((uint32_t)(0xC1) << 8)  // Prefix byte of address 1 converted to nRF24L series format
//       | ((uint32_t)(0xC0) << 0); // Prefix byte of address 0 converted to nRF24L series format
// /*
//
// (0x0AUL + PREFIX0NOTUSED )	|	// Not used
// 				(0x0AUL + MSG_TYPE_BEACON) << 8	|	// Beacon
// 				(0x0AUL + MSG_TYPE_ACK 	 ) << 16|	// Ack
// 				(0x0AUL + MSG_TYPE_SELECT) << 24;		// Select packet
// */
//     NRF_RADIO->BASE0 = 0x01234567UL;
//     NRF_RADIO->BASE1 = 0x89ABCDEFUL;
//
//     NRF_RADIO->RXADDRESSES = 	14UL;
// 				/*(1 << MSG_TYPE_BEACON) 	|
// 				(1 << MSG_TYPE_ACK   )	|
// 				(1 << MSG_TYPE_SELECT);    // 00001110*/
//
//     // Packet configuration
//     NRF_RADIO->PCNF0 = (PACKET_S1_FIELD_SIZE     << RADIO_PCNF0_S1LEN_Pos) |
//                        (PACKET_S0_FIELD_SIZE     << RADIO_PCNF0_S0LEN_Pos) |
//                        (PACKET_LENGTH_FIELD_SIZE << RADIO_PCNF0_LFLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"
//
//     // Packet configuration
//     NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
//                        (RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |
//                        (PACKET_BASE_ADDRESS_LENGTH   << RADIO_PCNF1_BALEN_Pos)   |
//                        (PACKET_STATIC_LENGTH         << RADIO_PCNF1_STATLEN_Pos) |
//                        (PACKET_PAYLOAD_MAXSIZE       << RADIO_PCNF1_MAXLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"
//
//     // CRC Config
//     NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits
//     NRF_RADIO->CRCINIT = 0xFFUL;        // Initial value
//     NRF_RADIO->CRCPOLY = 0x107UL;       // CRC poly: x^8+x^2^x^1+1
//
// }

uint32_t hash( uint8_t* pData, int count )
{
	uint32_t hash = 2166136261UL; // 32bit FNV offset
	int      i    = 0;

	for ( i = 0; i < count; i++ )
	{
		hash ^= (uint32_t)( pData[ i ] );
		hash *= 16777619UL; // 32bit FNV prime
	}

	return hash;
}


void send_sniffer(uint16_t src,uint16_t dst, uint16_t density1,uint16_t density2,uint8_t rssi){

	tx_power = RADIO_TXPOWER_TXPOWER_Pos4dBm;
	tx_channel = 80;
       NRF_RADIO->TXPOWER   = tx_power; 
       NRF_RADIO->FREQUENCY = tx_channel;
	
	// prepare packet
	snifferData.density = (density1 < 1023) ? density1 : 1023;
	snifferData.protocol = 1;
	snifferData.shortId = src;
	
	snifferData.anchor[0].rxOffset = 0;
	snifferData.anchor[0].rssi = (rssi < 63) ? rssi : 63;
	snifferData.anchor[0].anchor = dst;
	
	snifferData.anchor[1].rxOffset = 0;
	snifferData.anchor[1].rssi = 0;
	snifferData.anchor[1].anchor = (density2 < 1023) ? density2 : 1023;
	
	snifferData.anchor[2].rxOffset = 0;
	snifferData.anchor[2].rssi = 0;
	snifferData.anchor[2].anchor = 0;
	
	snifferData.validation = hash( (uint8_t*)(&snifferData), 12 );
	NRF_RADIO->PACKETPTR = (uint32_t)&snifferData;
    	// Switch to tx
	NRF_RADIO->TXADDRESS = 5; 
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

	tx_power = RADIO_TXPOWER_TXPOWER_0dBm;
	tx_channel = 75;
       NRF_RADIO->TXPOWER   = tx_power; 
       NRF_RADIO->FREQUENCY = tx_channel;
	
}
