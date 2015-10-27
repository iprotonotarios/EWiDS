#include "sofa.h"

/* These are set to zero to reduce overal packet size */
#define PACKET_S1_FIELD_SIZE             (0UL)  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             (0UL)  /**< Packet S0 field size in bits. */
#define PACKET_LENGTH_FIELD_SIZE         (0UL)  /**< Packet length field size in bits. */

#define PACKET_BASE_ADDRESS_LENGTH       (4UL)  //!< Packet base address length field size in bytes
#define PACKET_STATIC_LENGTH             (4UL)  //!< Packet static length in bytes
#define PACKET_PAYLOAD_MAXSIZE           (PACKET_STATIC_LENGTH)  //!< Packet payload maximum size in bytes

void radio_init()
{
    // Radio config
    NRF_RADIO->TXPOWER   = RADIO_TXPOWER_TXPOWER_Neg20dBm; // -20 dBm
    NRF_RADIO->FREQUENCY = 0UL;           // Frequency bin 0, 2400 Hz - before wifi
    NRF_RADIO->MODE      = RADIO_MODE_MODE_Nrf_2Mbit;

    // Radio address config
    NRF_RADIO->PREFIX0	= 	        ((uint32_t)(0xC3) << 24) // Prefix byte of address 3 converted to nRF24L series format
      | ((uint32_t)(0xC2) << 16) // Prefix byte of address 2 converted to nRF24L series format
      | ((uint32_t)(0xC1) << 8)  // Prefix byte of address 1 converted to nRF24L series format
      | ((uint32_t)(0xC0) << 0); // Prefix byte of address 0 converted to nRF24L series format
/*

(0x0AUL + PREFIX0NOTUSED )	|	// Not used
				(0x0AUL + MSG_TYPE_BEACON) << 8	|	// Beacon
				(0x0AUL + MSG_TYPE_ACK 	 ) << 16|	// Ack
				(0x0AUL + MSG_TYPE_SELECT) << 24;		// Select packet
*/
    NRF_RADIO->BASE0 = 0x01234567UL;  
    NRF_RADIO->BASE1 = 0x89ABCDEFUL;  

    NRF_RADIO->RXADDRESSES = 	14UL;
				/*(1 << MSG_TYPE_BEACON) 	|
				(1 << MSG_TYPE_ACK   )	|
				(1 << MSG_TYPE_SELECT);    // 00001110*/

    // Packet configuration
    NRF_RADIO->PCNF0 = (PACKET_S1_FIELD_SIZE     << RADIO_PCNF0_S1LEN_Pos) |
                       (PACKET_S0_FIELD_SIZE     << RADIO_PCNF0_S0LEN_Pos) |
                       (PACKET_LENGTH_FIELD_SIZE << RADIO_PCNF0_LFLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"

    // Packet configuration
    NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
                       (RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |
                       (PACKET_BASE_ADDRESS_LENGTH   << RADIO_PCNF1_BALEN_Pos)   |
                       (PACKET_STATIC_LENGTH         << RADIO_PCNF1_STATLEN_Pos) |
                       (PACKET_PAYLOAD_MAXSIZE       << RADIO_PCNF1_MAXLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"

    // CRC Config
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits
    NRF_RADIO->CRCINIT = 0xFFUL;        // Initial value
    NRF_RADIO->CRCPOLY = 0x107UL;       // CRC poly: x^8+x^2^x^1+1
    
}
