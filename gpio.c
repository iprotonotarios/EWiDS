#include "sofa.h"

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	NVIC_SystemReset();
}

void uart_error_handle(app_uart_evt_t * p_event)
{
}

void gpio_init(void)
{
    nrf_gpio_cfg_output(BSP_LED_0);
    nrf_gpio_cfg_output(BSP_LED_1);
    nrf_gpio_cfg_output(BSP_LED_2);
    nrf_gpio_cfg_output(GPIO_1);
    nrf_gpio_cfg_output(GPIO_2);
    nrf_gpio_cfg_output(GPIO_3);
    nrf_gpio_cfg_output(GPIO_5);

    nrf_gpio_pin_clear(BSP_LED_0);
    nrf_gpio_pin_clear(BSP_LED_2);
    
    nrf_gpio_cfg_input(PWREN_PIN, NRF_GPIO_PIN_PULLDOWN);
    
//   	nrf_gpio_pin_set(LED_YELLOW);

//  nrf_drv_gpiote_init();
//     nrf_drv_gpiote_in_config_t in_config_PWREN_PIN = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
//     nrf_drv_gpiote_in_init(PWREN_PIN, &in_config_PWREN_PIN, in_pin_handler);
//     nrf_drv_gpiote_in_event_enable(PWREN_PIN, true);
//
// // Remove for production version
//     nrf_drv_gpiote_in_config_t in_config_BUTTON_0 = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
//     nrf_drv_gpiote_in_init(BUTTON_0, &in_config_BUTTON_0, in_pin_handler);
//     nrf_drv_gpiote_in_event_enable(BUTTON_0, true);
//
//     const app_uart_comm_params_t comm_params =
//       {
//           RX_PIN_NUMBER,
//           TX_PIN_NUMBER,
//           RTS_PIN_NUMBER,
//           CTS_PIN_NUMBER,
//           APP_UART_FLOW_CONTROL_DISABLED,
//           false,
//           UART_BAUDRATE_BAUDRATE_Baud57600
//       };
//
//     uint32_t err_code;
//     APP_UART_FIFO_INIT(&comm_params,
//                     1,	//rx
//                     256,//tx
//                     uart_error_handle,
//                     APP_IRQ_PRIORITY_LOW,
//                     err_code);
//
//     if(err_code) nrf_gpio_pin_set(LED_RED);

}

