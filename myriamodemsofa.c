/*
 * ExampleBasicMyrianed.c
 *
 * Copyright (C) 2013 Van Mierlo Ingenieursbureau BV
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// external definitions
#include "mmLeds.h"
#include "mmButtons.h"
#include "mmDisplay.h"
#include "mmLetimer.h"
#include "mmLeuart.h"
#include "mmRadio.h"
#include "mmUnique.h"
#include "mmDebug.h"
#include "mmRtcTimer.h"
// BLE
#include "nRF8001.h"
#include "hal_aci_tl.h"

#define SNIFFER 0
#define DEBUG 0
#define BLE 0

#define S2Cycles 32768
typedef uint32_t nodeid_t;
typedef uint16_t tstamp_t;

#define TX_PWR_LOW 0
#define TX_PWR_HIGH 100

static nodeid_t mmNodeId;                //!< Node's "MAC" identifier

// Estreme definitions
struct EstremeLogS{
	uint16_t timestamp;
	uint16_t neighbor;
	uint8_t t_est;
	uint8_t s_est;
} __attribute__((packed));
typedef struct EstremeLogS EstremeLogT;
// Measuring the rendezvous samples
uint64_t t_rendezvous_time_start;   //!< Start of the rendezvous time measurement
uint64_t t_rendezvous_time;         //!< the rendezvous time measurement
// Averaging the rendezvous samples
#define SAMPLE_SIZE 25
uint64_t t_rendezvous_times[SAMPLE_SIZE];
uint16_t t_rendezvous_idx;
uint64_t t_rendezvous_sum;
uint64_t s_rendezvous_times[SAMPLE_SIZE];
uint16_t s_rendezvous_idx;
uint64_t s_rendezvous_sum;
// Logging
#define LOG_SIZE 1000
uint64_t log_start;
uint16_t pause_log;
uint16_t log_idx;
EstremeLogT log_estreme[LOG_SIZE];
// Count unique IDs
//#define MAX_IDS 10000
//bool ids[MAX_IDS];
//int ids_count = 0;

// SOFA definitions
// packet types
#define	SOFA_TYPE_BEACON 111
#define SOFA_TYPE_ACK 222
// Timing
#define SOFA_PERIOD      (S2Cycles)
#define STROBE_TIME      (SOFA_PERIOD) //(SOFA_PERIOD/2)
#define ACKWAIT_TIME     SOFA_PERIOD/512 //(SOFA_PERIOD/1024) // 512
#define LISTENING_TIME   (ACKWAIT_TIME*3) //(SOFA_PERIOD/10) //(SOFA_PERIOD/1024)
#if SOFA_PERIOD > ((((1 << 16) - 1) * 2) / 3)
#error SOFA_PERIOD too large for 16-bit timer!
#endif
// Radio packet
struct SofaMessageS{
	uint16_t hdr;			// added for use with OTAP bootloader
	//uint16_t padding;
	uint32_t src;
	uint32_t dst;
	uint32_t type;
	uint32_t estreme_average;
	uint32_t estreme_delay;
	uint32_t other_estreme;  //Marco ICTOpen change
	uint8_t unused[6];       //Marco ICTOpen change
	//uint8_t unused[10];	 // updated after addition of hdr element
} __attribute__((packed));
typedef struct SofaMessageS SofaMessageT;
// FSM
typedef enum SofaFsmStateEnum {
	SOFA_STATE_START = 0,
	SOFA_STATE_LISTEN,
	SOFA_STATE_BEACONPREP,
	SOFA_STATE_BEACONSEND,
	SOFA_STATE_ACKWAIT,
	SOFA_STATE_ACKPREP,
	SOFA_STATE_ACKSEND,
	SOFA_STATE_IDLE,
	SOFA_STATE_SLEEP,
	NUM_SOFA_FSM_STATES,
} SofaFsmState;
// Radio states
typedef enum SofaRadioStateEnum {
	SOFA_RADIO_RX = 0,
	SOFA_RADIO_TX,
	SOFA_RADIO_SB,
	SOFA_RADIO_PD,
	NUM_SOFA_RADIO_STATES,
} SofaRadioState;
// Variables
volatile SofaFsmState sofa_fsm_state;
volatile SofaRadioState sofa_radio_state;
static volatile uint8_t radioResourceLock; //!< 1 = The SPI interface of the radio is in use for Read Rx FIFO
static volatile uint8_t radioCmdQueue;     //!< Holds Radio command when the \ref radioResourceLock is set
volatile uint8_t msg_received;      //!< Records whether a message has been received

// IO
char buffer[14]; //buffer for string display
char buffer_serial[40]; //buffer for serial output

// Functions

static inline uint8_t sofaSwitchTo(uint8_t mode) {
	switch (mode) {
	case RADIO_RXMODE:
		// If the radio isn't in RX, turn radio to RX
		if (sofa_radio_state != SOFA_RADIO_RX) {
			radioCommand(RADIO_DISABLE);
#if DEBUG
			mmDEBUG0(DEBUG_ON);
			mmDEBUG1(DEBUG_OFF);
#endif
			radioCommand(RADIO_RXMODE); // set radio in receive mode
			sofa_radio_state = SOFA_RADIO_RX;
			radioCommand(RADIO_ENABLE);
			radioCommand(RADIO_IRQACK);
		}
		break;
	case RADIO_TXMODE:
		// If the radio isn't in TX, turn radio to TX
		if (sofa_radio_state != SOFA_RADIO_TX) {
			if (radioResourceLock) {
				radioCmdQueue = RADIO_TXMODE;
				return 0;
			}
			//MC radioCommand(RADIO_DISABLE);
#if DEBUG
			mmDEBUG0(DEBUG_OFF);
			mmDEBUG1(DEBUG_ON);
#endif
			radioCommand(RADIO_TXMODE); // set radio in transmitt mode
			sofa_radio_state = SOFA_RADIO_TX;
			//radioCommand(RADIO_ENABLE);
		}
		break;
	case RADIO_SBMODE:
		// If the radio isn't in standby, turn radio to standby
		if (sofa_radio_state != SOFA_RADIO_SB) {
			if (radioResourceLock) {
				radioCmdQueue = RADIO_SBMODE;
				return 0;
			}
			//clear TX,RX signals
#if DEBUG
			mmDEBUG0(DEBUG_OFF);
			mmDEBUG1(DEBUG_OFF);
#endif
			radioCommand(RADIO_SBMODE);
			sofa_radio_state = SOFA_RADIO_SB;
		}
		break;
	case RADIO_PDMODE:
		// If the radio isn't in standby, turn radio to standby
		if (sofa_radio_state != SOFA_RADIO_PD) {
			//clear TX,RX signals
#if DEBUG
			mmDEBUG0(DEBUG_OFF);
			mmDEBUG1(DEBUG_OFF);
#endif
			//			mcRadioCommand(PDMODE);
			sofa_radio_state = SOFA_RADIO_PD;
		}
		break;
	default:
		// Unknown radio mode
		return 0;
	}
	return 1;
}

int rtcTimerIntHandler(void){
	//mmDEBUG0(DEBUG_TOGGLE);
	return 0;
}

/* Initialize Estreme's structures */
void estreme_init(){
	int i;
	for(i=0;i<SAMPLE_SIZE;i++){
		t_rendezvous_times[i]=1000;
		s_rendezvous_times[i]=1000;
	}
	//	for(i=0;i<MAX_IDS;i++){
	//		ids[i]=false;
	//	}
	t_rendezvous_idx = 0;
	t_rendezvous_sum = SAMPLE_SIZE*1000;
	s_rendezvous_idx = 0;
	s_rendezvous_sum = SAMPLE_SIZE*1000;
	//logging
	pause_log = 0;
	log_idx = 0;
	for(i=0;i<LOG_SIZE;i++){
		log_estreme[log_idx].timestamp = 0;
		log_estreme[log_idx].neighbor = 0;
		log_estreme[log_idx].t_est = 0;
		log_estreme[log_idx].s_est = 0;
	}
}
/* add a new value and return the average rendezvous length in milliseconds*10 */
uint64_t t_estreme_update(uint64_t new_value){
	// if rendezvous is greater than wakeup period (1 second) ignore it
	if(new_value > 10000) return 0;
	// if rendezvous is 0, ignore it
	if(new_value == 0) return 0;
	// update the average window
	t_rendezvous_sum -= t_rendezvous_times[t_rendezvous_idx];
	t_rendezvous_times[t_rendezvous_idx] = new_value;
	t_rendezvous_sum += new_value;
	t_rendezvous_idx = ((t_rendezvous_idx+1)%SAMPLE_SIZE);
	return (t_rendezvous_sum/SAMPLE_SIZE);
}
/* add a new value and return the average rendezvous length in milliseconds*10 */
uint64_t s_estreme_update(uint64_t new_value){
	// if rendezvous is greater than wakeup period (1 second) ignore it
	if(new_value > 10000) return 0;
	// if rendezvous is 0, ignore it
	if(new_value == 0) return 0;
	// update the average window
	s_rendezvous_sum -= s_rendezvous_times[s_rendezvous_idx];
	s_rendezvous_times[s_rendezvous_idx] = new_value;
	s_rendezvous_sum += new_value;
	s_rendezvous_idx = ((s_rendezvous_idx+1)%SAMPLE_SIZE);
	return (s_rendezvous_sum/SAMPLE_SIZE);
}
/* return the average rendezvous length in ticks*10 */
uint64_t t_get_average(){
	return (t_rendezvous_sum/SAMPLE_SIZE);
}
/* return the average rendezvous length in ticks*10 */
uint64_t s_get_average(){
	return (s_rendezvous_sum/SAMPLE_SIZE);
}
/* return the estimated cardinality*10 */
uint32_t t_get_estimate(){
	return (uint32_t)((10000/t_get_average())-1);
}
/* return the estimated cardinality*10 */
uint32_t s_get_estimate(){
	return (uint32_t)((10000/s_get_average())-1);
}

void handleRX(){
	static SofaMessageT TempRxMsg;
	static SofaMessageT *pTempRxMsg;
	pTempRxMsg = &TempRxMsg;

	//read message without popping from radio fifo
	radioRxFifoRead((uint32_t *)&TempRxMsg);
	// if the message is not broadcasted and is not for us, ignore
	// this is the case for example of ACK that are transmitted at higher power
	if ((pTempRxMsg->dst==mmNodeId) || (pTempRxMsg->dst==0)){
//		mmLeuartPutStr("Type: ");
//		mmLeuartPutDec(pTempRxMsg->type);
//		mmLeuartPutStr(" Dst: ");
//		mmLeuartPutChar((pTempRxMsg->dst)>>8);
//		mmLeuartPutChar(pTempRxMsg->dst);
//		mmLeuartPutStr("\r\n");
		msg_received = 1;
	}
	else{
		//mmLeuartPutStr("Message not for me!!!!!!\r\n");
	}
}

// main program function
int main(void)
{
	static SofaMessageT SofaTxMsg, SofaRxMsg;
	static SofaMessageT *pSofaTxMsg, *pSofaRxMsg;
	static tstamp_t mcRoundNum = 0;          //!< Current global round number
	static uint64_t next_period_start;       //!< Random duration of this SOFA period
	static uint64_t stop_time, wait_time;
	pSofaTxMsg = &SofaTxMsg;
	pSofaRxMsg = &SofaRxMsg;
	// initialize external peripherals
	mmLedsInit();
#if DEBUG
	mmDEBUGInit();
#endif

	mmLedsSet(LEDS_ALL);
	mmButtonsInit();
	mmDisplayInit();
	mmLeuartInit(LEUART_B9600, LEUART_PAR_NONE, LEUART_1STOPBIT);
	rtcTimerInit();
	mmNodeId = mmUniqueId();




	srand(mmNodeId);
	// init radio
	//	char radioaddress[5] = {0x35, 0x21, 0x5d, 0xab, 0xcd};
	radioInit((void *)handleRX,mmNodeId);
	radioChannel(82);
	radioPower(TX_PWR_LOW);

#if BLE
	mmLeuartPutStr("Entering BLE\r\n");
	mmnRF8001Init();
	mmLeuartPutStr("Exiting BLE\r\n");

#endif

	//radioPower(0);
	//	radioAddress(radioaddress);			// use default address for use with OTAP bootloader
#if SNIFFER
	uint32_t tmp_average,tmp_delay,tmp_count,tmp_id;
	uint64_t log_start,log_timestamp;

	tmp_average=0;
	tmp_delay=0;
	tmp_id = 0;
	tmp_count=0;
	/* show text on screen */
	if (mmDisplayPresent()) {
		mmDisplayText(0, 0, "M.Modem");
		mmDisplayText(1, 0, "Sniffer");
	}
	radioResourceLock = 0;
	sofa_fsm_state = SOFA_STATE_IDLE;
	radioCmdQueue = RADIO_NOP;
	sofaSwitchTo(RADIO_SBMODE);
	sofa_fsm_state = SOFA_STATE_LISTEN;
	mmLedsSet(LEDS_ORANGE);
	//while(!mmButtonsPressedA());
	log_start = rtcUptime();
	mmLedsSet(LEDS_GREEN);
	// clean the radio buffer!
	while(!radioRxFifoEmpty()){
		radioRxFifoPop((uint32_t *)&SofaRxMsg);
	}
	radioCommand(RADIO_FLUSHRX); // Remove any possible RX messages that could still be in the radio
	radioCommand(RADIO_FLUSHTX); // Remove any possible TX messages that could still be in the radio
	msg_received = 0;
	sofaSwitchTo(RADIO_RXMODE);
	mmLeuartPutStr("Sniffer ready\r\n");
	/* the looop! */
	while(1){
		mmLedsSet(LEDS_GREEN);
		mmDEBUG2(DEBUG_ON);
		wait_time = rtcUptime() + S2Cycles;
		while (!msg_received && (rtcUptime() < wait_time)){
		};
		mmDEBUG2(DEBUG_OFF);
		/* If a beacon is received in the backoff, send an ACK and go to sleep */
		if (msg_received) {
			mmLedsSet(LEDS_RED);
			mmDEBUG3(DEBUG_ON);
			while(radioRxFifoEmpty()); // wait for received packet
			radioRxFifoPop((uint32_t *)&SofaRxMsg);

			if (pSofaRxMsg->type==SOFA_TYPE_ACK)
			{
				//print timestamp
				mmLeuartPutStr("\r\n");
				log_timestamp = rtcUptime()-log_start;
				//mmLeuartPutDec((uint16_t)(log_timestamp/(S2Cycles/10)));
				//mmLeuartPutDec((uint16_t)tmp_count);
				//mmLeuartPutStr(" 2");
				//mmLeuartPutStr(" ");
				//mmLeuartPutStr(" A ");
				mmLeuartPutChar((pSofaRxMsg->src)>>8);
				mmLeuartPutChar(pSofaRxMsg->src);
				mmLeuartPutStr(" ");
				mmLeuartPutChar((pSofaRxMsg->dst)>>8);
				mmLeuartPutChar(pSofaRxMsg->dst);
				snprintf(buffer_serial,40," %u",(uint16_t)(pSofaRxMsg->estreme_average));
				mmLeuartPutStr(buffer_serial);
				snprintf(buffer_serial,40," %u",(uint16_t)(pSofaRxMsg->other_estreme)); //Marco ICTOpen
				mmLeuartPutStr(buffer_serial);

//				snprintf(buffer_serial,40," %u %u %u",(uint16_t)((pSofaRxMsg->src)%(65535)),(uint16_t)((pSofaRxMsg->dst)%(65535)),(uint16_t)(pSofaRxMsg->estreme_average));
//				mmLeuartPutStr(buffer_serial);
			}
			else{
				// print beacon only if it is different than previous ones
				if((tmp_id!=pSofaRxMsg->src) || (tmp_average!=pSofaRxMsg->estreme_average) || (tmp_delay != pSofaRxMsg->estreme_delay)){
					//print timestamp
//					mmLeuartPutStr("\r\n");
					log_timestamp = rtcUptime()-log_start;
//					mmLeuartPutDec((uint16_t)(log_timestamp/(S2Cycles/10)));
					tmp_count = 0;
					tmp_average = pSofaRxMsg->estreme_average;
					tmp_delay = pSofaRxMsg->estreme_delay;
					tmp_id = pSofaRxMsg->src;

//					mmLeuartPutStr(" B ");
//					mmLeuartPutChar((pSofaRxMsg->src)>>8);
//					mmLeuartPutChar(pSofaRxMsg->src);
//					mmLeuartPutStr(" - ");
//					mmLeuartPutChar((pSofaRxMsg->dst)>>8);
//					mmLeuartPutChar(pSofaRxMsg->dst);
//					snprintf(buffer_serial,40," %u",(uint16_t)(pSofaRxMsg->estreme_average));

//					mmLeuartPutStr(" 1");
//					snprintf(buffer_serial,40," %u %u %u",(uint16_t)((pSofaRxMsg->src)%(65535)),(uint16_t)((pSofaRxMsg->dst)%(65535)),(uint16_t)(pSofaRxMsg->estreme_average));
//					mmLeuartPutStr(buffer_serial);
				}else{
//					tmp_count++;
//					if ((tmp_count%10)==0)
//						mmLeuartPutStr(".");
				}
			}
			mmDEBUG3(DEBUG_OFF);
		}
	}
#else
	// wait for button A to start experiment
//	while(1){
//		if (mmButtonsPressedA()) break;
//	}

	//while(!mmButtonsPressedA());

	//mmLeuartPutChar(mmNodeId>>8);
	//mmLeuartPutChar(mmNodeId);
	//mmLeuartPutStr("\r\n");

	estreme_init();
	pause_log = 0;
	/* show text on screen */
	if (mmDisplayPresent()) {
		mmDisplayText(0, 0, "M.Modem");
		mmDisplayText(1, 0, "Estreme");
	}
	radioResourceLock = 0;
	// Logging start
	log_start = rtcUptime();
	// the looop!
	while(1){
#if with_seriallogs
		if (mmButtonsPressedB() ) {
			if (mmDisplayPresent()) {
				mmDisplayText(0, 0, "Paused");
				mmDisplayText(1, 0, "Estreme");
			}
			sofa_fsm_state = SOFA_STATE_IDLE;
			radioCmdQueue = RADIO_NOP;
			sofaSwitchTo(RADIO_SBMODE);
			mmLedsSet(LEDS_OFF);
			while(1){
				int i;
				while(!mmButtonsPressedA());
				for(i=0;i<LOG_SIZE;i++){
					if ((log_estreme[(i)%LOG_SIZE].neighbor) == 0) break;
					mmLetimerDelayMs(25);
					mmLeuartPutDec(log_estreme[(i)%LOG_SIZE].timestamp);
					mmLeuartPutStr(" ");
					mmLeuartPutChar(mmNodeId);
					mmLeuartPutChar(mmNodeId>>8);
					//mmLeuartPutDec((mmNodeId)%(65535));
					mmLeuartPutStr(" ");
					mmLeuartPutChar((log_estreme[(i)%LOG_SIZE].neighbor)>>8);
					mmLeuartPutChar(log_estreme[(i)%LOG_SIZE].neighbor);
					//mmLeuartPutDec(log_estreme[(i)%LOG_SIZE].neighbor);
					mmLeuartPutStr(" ");
					mmLeuartPutDec(log_estreme[(i)%LOG_SIZE].t_est);
					mmLeuartPutStr(" ");
					mmLeuartPutDec(log_estreme[(i)%LOG_SIZE].s_est);
					mmLeuartPutStr("\r\n");
				}
			}
		}
#endif
//		if (mmButtonsPressedB()) {
//			int i;
//			//output log
//			//log_idx = 0;
//			mmLeuartPutStr("Waiting for log...\r\n");
//			for(i=0;i<LOG_SIZE;i++){
//				mmLetimerDelayMs(25);
////				if (log_estreme[(log_idx+i)%LOG_SIZE].timestamp == 0) {
////					continue;
////				}
//				mmLeuartPutDec(log_estreme[i%LOG_SIZE].timestamp);
//				mmLeuartPutStr(" ");
//				mmLeuartPutDec((mmNodeId)%(65535));
//				mmLeuartPutStr(" ");
//				mmLeuartPutDec(log_estreme[i%LOG_SIZE].neighbor);
//				mmLeuartPutStr(" ");
//				mmLeuartPutDec(log_estreme[i%LOG_SIZE].t_est);
//				mmLeuartPutStr(" ");
//				mmLeuartPutDec(log_estreme[i%LOG_SIZE].s_est);
//				mmLeuartPutStr("\r\n");
//			}
//		}
		mmLedsSet(LEDS_NONE);
		++mcRoundNum;
		//set here the next wakeup in [0.5,1.5] seconds
		next_period_start = rtcUptime() + S2Cycles/2 + (rand()%S2Cycles);
		//next_period_start = rtcUptime() + S2Cycles;
		// back-off
		t_rendezvous_time_start = rtcUptime();
#if DEBUG
		mmDEBUG2(DEBUG_ON);
#endif
		sofa_fsm_state = SOFA_STATE_LISTEN;
		mmLedsSet(LEDS_GREEN);
		// clean the radio buffer!
		while(!radioRxFifoEmpty()){
			radioRxFifoPop((uint32_t *)&SofaRxMsg);
		}
		radioCommand(RADIO_FLUSHRX); // Remove any possible RX messages that could still be in the radio
		radioCommand(RADIO_FLUSHTX); // Remove any possible TX messages that could still be in the radio
		msg_received = 0;
		sofaSwitchTo(RADIO_RXMODE);
		// stop after LISTENING TIME or when receive a message
		wait_time = rtcUptime() + (uint64_t)LISTENING_TIME;
		while (!msg_received && (rtcUptime() < wait_time)){
		};
#if DEBUG
		mmDEBUG2(DEBUG_OFF);
#endif
		mmLedsRed(LEDS_OFF);
		/* If a beacon is received in the backoff, send an ACK and go to sleep */
		if (msg_received) {
			//			uint64_t temp_rendezvous;
			//			uint32_t *idx;
			//			int j;

			while(radioRxFifoEmpty()); // wait for received packet
			radioRxFifoPop((uint32_t *)&SofaRxMsg);

			if (pSofaRxMsg->type==SOFA_TYPE_BEACON)
			{
				//update unique count
				//				if (ids[((pSofaRxMsg->src)%MAX_IDS)]!=true)
				//				{
				//					ids[((pSofaRxMsg->src)%MAX_IDS)]=true;
				//					ids_count++;
				//				}
				//update S-Estreme
				s_estreme_update(pSofaRxMsg->estreme_average);
				// SEND ACK
				mmLedsOrange(LEDS_OFF);
				msg_received = 0;
				// Prepare and Load our ACK message into the radio
				pSofaTxMsg->hdr  = 0x8000;					// for use with OTAP bootloader
				pSofaTxMsg->type = SOFA_TYPE_ACK;
				pSofaTxMsg->src  = mmNodeId;
				//pSofaTxMsg->src  = ((mmNodeId)%(65535));
				pSofaTxMsg->dst  = pSofaRxMsg->src;
				pSofaTxMsg->estreme_average = t_get_average();
				pSofaTxMsg->other_estreme = pSofaRxMsg->estreme_average; //Marco ICTOpen
				//Marco: send the beacon at higher power
				radioPower(TX_PWR_HIGH);
				// Send the message
				//mmDelayUs(300); // requires less then 1 ms
				while (radioResourceLock)
					;
				//save delay
				t_rendezvous_time = rtcUptime() - t_rendezvous_time_start;
				pSofaTxMsg->estreme_delay = (uint32_t)t_rendezvous_time;
				// copy packet into buffer
				radioTransmitPacket((const uint8_t *)pSofaTxMsg, RADIO_PAYLOAD_SIZE);
				// do transmit!
				sofaSwitchTo(RADIO_TXMODE);
				radioTransmit(); // transmit packet
				mmLetimerDelayUs(300); // requires less then 1 ms
				//				idx=(uint32_t *)pSofaTxMsg;
				//				for (j=0; j<8; j++){
				//					uint32_t temp;
				//					temp = *(idx+j);
				//					mmLeuartPutHex8(temp);
				//					mmLeuartPutStr(" ");
				//				}

				//Marco: go back to normal transmission power
				radioPower(TX_PWR_LOW);

			} //endif beacon received
			else{
				// we were waiting for a beacon but we received something else
			}
		} //endif message received
		else {
			mmLedsSet(LEDS_RED);
			// STROBE BEACON
			sofa_fsm_state = SOFA_STATE_BEACONSEND;
			msg_received = 0;
			//mmDEBUG0(DEBUG_ON); // Set Beacon signal
			/* stop after LISTENING TIME or when receive a message */
			stop_time = rtcUptime() + (uint64_t)STROBE_TIME;

			while (!msg_received && (rtcUptime() < stop_time)) {
				//mmLedsGreen(LEDS_TOGGLE);
				// prepare packet and load
				pSofaTxMsg->hdr  = 0x8000;					// for use with OTAP bootloader
				pSofaTxMsg->type = SOFA_TYPE_BEACON;
				pSofaTxMsg->src  = mmNodeId;
				//pSofaTxMsg->src  = ((mmNodeId)%(65535));
				pSofaTxMsg->dst  = 0;
				pSofaTxMsg->estreme_average = t_get_average();
				//mmDEBUG1(DEBUG_ON);
				while (radioResourceLock)
					;
				radioTransmitPacket((const uint8_t *)pSofaTxMsg, RADIO_PAYLOAD_SIZE);
				//mmDEBUG0(DEBUG_OFF);
				//mmDEBUG1(DEBUG_ON);
				sofaSwitchTo(RADIO_TXMODE);
				//MC radioCommand(RADIO_TXMODE); // set radio in transmit mode
				radioTransmit(); // transmit packet
				mmLetimerDelayUs(300); // requires less then 1 ms
				//mmDEBUG1(DEBUG_OFF);
				// Wait for a possible ACK message
				sofa_fsm_state = SOFA_STATE_ACKWAIT;
				radioCommand(RADIO_FLUSHRX); // Remove any possible RX messages that could still be in the radio

				//mmDEBUG0(DEBUG_ON);
				//mmDEBUG1(DEBUG_OFF);
				sofaSwitchTo(RADIO_RXMODE);
				//radioCommand(RADIO_RXMODE); // set radio in transmit mode
				//radioCommand(RADIO_ENABLE);
				wait_time = rtcUptime() + (uint64_t)ACKWAIT_TIME;
				while (!msg_received && (rtcUptime() < wait_time))
					;
				sofa_fsm_state = SOFA_STATE_BEACONSEND;
			}
			//mmDEBUG0(DEBUG_OFF);

			// if an ACK was received, save the rendezvous and go back to sleep
			if (msg_received) {
				//				uint32_t *ptemp,*idx;
				//				int j;
				uint64_t temp_rendezvous,log_timestamp;
				//mmLedsGreen(LEDS_ON);
				//stop the measurement
				t_rendezvous_time = rtcUptime() - t_rendezvous_time_start;
				while(radioRxFifoEmpty()); // wait for received packet
				radioRxFifoPop((uint32_t *)pSofaRxMsg);

//				mmLeuartPutStr("APP - Dst: ");
//				mmLeuartPutChar((pSofaRxMsg->dst)>>8);
//				mmLeuartPutChar(pSofaRxMsg->dst);
//				mmLeuartPutStr("\r\n");

//				if (pSofaRxMsg->type==SOFA_TYPE_ACK)
				if ((pSofaRxMsg->type==SOFA_TYPE_ACK)&&(pSofaRxMsg->dst==mmNodeId))
				{
					//				//shift pointer by 32bit (bug?)
					//				ptemp = (uint32_t *)pSofaRxMsg;
					//				++ptemp;
					//				pSofaRxMsg = (SofaMessageT *)ptemp;

					//				//print received packet
					//				 mmLeuartPutStr("RxACK ");
					//				idx=(uint32_t *)pSofaRxMsg;
					//				for (j=0; j<8; j++){
					//					uint32_t temp;
					//					temp = *(idx+j);
					//					mmLeuartPutHex8(temp);
					//					mmLeuartPutStr(" ");
					//				}

					//update unique count
					//					if (ids[((pSofaRxMsg->src)%MAX_IDS)]!=true)
					//					{
					//						ids[((pSofaRxMsg->src)%MAX_IDS)]=true;
					//						ids_count++;
					//					}
					s_estreme_update(pSofaRxMsg->estreme_average);

					temp_rendezvous = (t_rendezvous_time)-(uint64_t)(pSofaRxMsg->estreme_delay);
					temp_rendezvous *= 10000;
					//mmLeuartPutDec((uint16_t)((temp_rendezvous/32768)-10));
					t_estreme_update((temp_rendezvous/32768)-10);
					// Log estimations
					//if(pause_log==0){

						log_timestamp = rtcUptime()-log_start;
						log_estreme[log_idx].timestamp = (uint32_t)(log_timestamp/S2Cycles);
						log_estreme[log_idx].neighbor = (uint16_t)pSofaRxMsg->src;
						log_estreme[log_idx].t_est = (uint8_t)t_get_estimate();
						log_estreme[log_idx].s_est = (uint8_t)s_get_estimate();
						log_idx = ((log_idx+1)%LOG_SIZE);

					//}
					if (mmDisplayPresent()) {
						char buffer[14];
						//					int id;
						//					id = pSofaRxMsg->src;
						mmDisplayClear();
						//					snprintf(buffer,14,"T %u",(uint16_t)t_get_average());
						//					mmDisplayText(0, 0,buffer);
						//					snprintf(buffer,14,"S %u",(uint16_t)s_get_average());
						//					mmDisplayText(1, 0,buffer);
						snprintf(buffer,14,"T %u",(uint16_t)(log_timestamp/S2Cycles));
						mmDisplayText(0, 0,buffer);
						//snprintf(buffer,14,"T %uS%u",(uint16_t)t_get_estimate(),(uint16_t)s_get_estimate());
						snprintf(buffer,14,"D %u",(uint16_t)t_get_estimate());
						//snprintf(buffer,14,"U %u",(uint16_t)ids_count);
						mmDisplayText(1, 0,buffer);
					}
				} //endif we received an ack
				else
				{
					//we were waiting for an ack but we received something else
				}
			} //endif we received a message
			else {
			}
		}
		//if(pause_log==0){
			mmLedsSet(LEDS_NONE);
//		}else{
//			mmLedsSet(LEDS_ALL);
//		}
		// go to sleep!
		sofa_fsm_state = SOFA_STATE_IDLE;
		radioCmdQueue = RADIO_NOP;
		sofaSwitchTo(RADIO_SBMODE);
		//		mmDisplayClear();
		//		char buffer[14];
		//		snprintf(buffer,14,"E %u",t_get_estimate());
		//		mmDisplayText(0, 0,buffer);
		//		snprintf(buffer,14,"R %u",mcRoundNum);
		//		mmDisplayText(1, 0,buffer);

		//		mmLeuartPutStr("E");
		//		mmLeuartPutDec((uint16_t)(t_get_estimate()));
		//		mmLeuartPutStr(" R");
		//		mmLeuartPutDec(mcRoundNum);
		//		mmLeuartPutStr("\n");

#if BLE
		uint8_t density;
		density = (uint8_t)(t_get_estimate()%255);
		mmBleSendData(&density, 1);
		hal_aci_tl_poll_rdy_line();
#endif
		// If the next period already elapsed, add another delay [0.5,1.5]
		while (next_period_start < rtcUptime())
			next_period_start = next_period_start + S2Cycles/2 + (rand()%S2Cycles);
		//next_period_start = next_period_start + S2Cycles;

		//mmLetimerDelayMs(next_period_start-rtcMsUptime());
		//wait until the wake-up period arrives
		while(next_period_start>rtcUptime()){};

	}
#endif
}
