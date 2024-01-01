#ifndef MOTOR_HPP
#define MOTOR_HPP

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

/*
 * Function: pump_init
 * --------------------
 * Initialises GPIO and ADC for
 *
 * motorControlPin: GPIO pin which will be set as output, to control the H-Bridge
 * motorAdcPin: GPIO to be setup as ADC input which will be used to detect when
 *              the pump is dry
 *
 * returns: void
 */
void pump_init( uint8_t pumpControlPin, uint8_t pumpAdcPin );

#endif // define MOTOR_HPP
