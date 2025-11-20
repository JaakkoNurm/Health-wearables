#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nordic_common.h"
#include "boards.h"
#include "nrf_drv_timer.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_drv_gpiote.h"
#include "nrf_temp.h"

// Exercise 2
#include "nrfx_twi.h"
#include "lsm6dso_reg.h"
#include "lsm6dso_platform.h"

const nrf_drv_timer_t TIMER_TEMP = NRF_DRV_TIMER_INSTANCE(0);


/**
 * @brief Function for application main entry.
 */

 #define BUTTON_PIN 13

 int32_t volatile temp;

/**
* Function for reading on-die temperature
*/
void meas_temp()
{
  NRF_TEMP->TASKS_START = 1; // Start the temperature measurement
  // Busy wait while temperature measurement is not finished
  while (NRF_TEMP->EVENTS_DATARDY == 0)
  {
    // Do nothing.
  }
  NRF_TEMP->EVENTS_DATARDY = 0;
  // The value in the register is a multiple of 4
  temp = (nrf_temp_read() / 4);
  NRF_TEMP->TASKS_STOP = 1; // Stop the temperature measurement
}
 
// Handler for timer events.
void temp_timeout_handler(nrf_timer_event_t event_type, void* p_context)
{
  meas_temp();
}

// Handler for GPIO events.
void button_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  printf("button was pressed\n");
  printf("\n%ld", temp);
  meas_temp();
}

 // Function for initializing the timer.
 uint32_t init_timers()
    {
      uint32_t time_ms = 5000;
      uint32_t time_ticks;
      uint32_t err_code = NRF_SUCCESS;
      nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
      err_code = nrf_drv_timer_init(&TIMER_TEMP, &timer_cfg, temp_timeout_handler);
      APP_ERROR_CHECK(err_code);
      time_ticks = nrf_drv_timer_ms_to_ticks(&TIMER_TEMP, time_ms);
      nrf_drv_timer_extended_compare(&TIMER_TEMP, NRF_TIMER_CC_CHANNEL0, time_ticks,
      NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
      nrf_drv_timer_enable(&TIMER_TEMP);
      return err_code;
    }

uint32_t config_gpio()
{
  uint32_t err_code = NRF_SUCCESS;
  if(!nrf_drv_gpiote_is_init())
  {
    err_code = nrf_drv_gpiote_init();
  }
  // Set which clock edge triggers the interrupt
  nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
  // Configure the internal pull up resistor
  config.pull = NRF_GPIO_PIN_PULLUP;
  // Configure the pin as input
  err_code = nrf_drv_gpiote_in_init(BUTTON_PIN, &config, button_handler);
  if (err_code != NRF_SUCCESS)
  {
    // handle error condition
    printf(err_code, "\n");
  }
  // Enable events
  nrf_drv_gpiote_in_event_enable(BUTTON_PIN, true);
  return err_code;
}

#define TWI_INSTANCE_ID 0

const nrfx_twi_t m_twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

void twi_init(void)
{
  nrfx_twi_config_t twi_config = NRFX_TWI_DEFAULT_CONFIG;
  twi_config.scl = 27; // Arduino pins
  twi_config.sda = 26;
  twi_config.frequency = NRF_TWI_FREQ_400K;

  nrfx_twi_init(&m_twi, &twi_config, twi_handler, NULL);
  nrfx_twi_enable(&m_twi);
}

/**
* Function for configuring the sensor
*/
stmdev_ctx_t dev_ctx;


void lsm6dso_setup(void)
{
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay_ms;
  dev_ctx.handle = (void*)&m_twi; // pass TWI instance handle

  // Sensor id check
  uint8_t whoamI = 0;
  lsm6dso_device_id_get(&dev_ctx, &whoamI);
  if (whoamI != LSM6DSO_ID) {
    printf("Error: sensor not detected\n");
    while(1);
  }
  printf("Sensor ID: %#02x\n", whoamI);
}


int main(void)
{
    config_gpio();
    init_timers();
    twi_init();
    lsm6dso_setup();
    
    
    while (true)
    {
        // do nothing
    }
}

