/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef WRISTBAND_H
#define WRISTBAND_H

#define LEDS_NUMBER    3

#define LED_START      14
#define LED_RED        14
#define LED_YELLOW     15
#define LED_GREEN      16
#define LED_STOP       16

#define BSP_LED_0 LED_RED
#define BSP_LED_1 LED_YELLOW
#define BSP_LED_2 LED_GREEN

#define BSP_LED_0_MASK    (1<<BSP_LED_0)
#define BSP_LED_1_MASK    (1<<BSP_LED_1)
#define BSP_LED_2_MASK    (1<<BSP_LED_2)
#define LEDS_MASK      (BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK)
#define LEDS_INV_MASK  0x00000000 //or LEDS_MASK

#define LEDS_LIST { LED_RED, LED_YELLOW, LED_GREEN}

#define GPIO_1	2 
#define GPIO_2	28 
#define GPIO_3	29 
#define GPIO_4	30 
#define GPIO_5	1 
#define GPIO_6	0

#define BUTTON_0       12
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BSP_BUTTON_0   BUTTON_0

#define BUTTONS_NUMBER 1

#define BSP_BUTTON_0_MASK (1<<BUTTON_0)
#define BUTTONS_MASK   BSP_BUTTON_0_MASK 

#define BUTTONS_LIST { BUTTON_0 }

#define RX_PIN_NUMBER  10    // UART RX pin number.
#define TX_PIN_NUMBER  11    // UART TX pin number.
#define CTS_PIN_NUMBER 99
#define RTS_PIN_NUMBER 98
#define HWFC           false // UART hardware flow control.

#define BCD_PIN		9
#define PWREN_PIN	8

#define SPIS_MISO_PIN  5    // SPI MISO signal. 
#define SPIS_CSN_PIN   4    // SPI CSN signal. 
#define SPIS_MOSI_PIN  6    // SPI MOSI signal. 
#define SPIS_SCK_PIN   7    // SPI SCK signal. 

#endif  // WRISTBAND_H
