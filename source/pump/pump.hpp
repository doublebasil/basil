#ifndef MOTOR_HPP
#define MOTOR_HPP

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "settings.hpp"

/*
 * Function: pump_init
 * --------------------
 * Initialises GPIO and ADC for
 *
 * globalDataPtr: pointer to the global data struct in main
 * motorControlPin: GPIO pin which will be set as output, to control the H-Bridge
 * motorAdcPin: GPIO to be setup as ADC input which will be used to detect when
 *              the pump is dry
 *
 * returns: void
 */
void pump_init( uint8_t pumpControlPin, uint8_t pumpAdcPin );

/*
 * Function: pump_runMs
 * --------------------
 * Run the pump for the number of milliseconds specified in the global data struct, 
 * or until a stall is detected
 *
 * returns: void
 */
void pump_run( void );

#endif // define MOTOR_HPP
