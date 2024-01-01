#include "pump.hpp"

#include "stdio.h"

static uint8_t m_pumpControlPin;

void pump_init( uint8_t pumpControlPin, uint8_t pumpAdcPin )
{
    // Initialise the motor control pin as GPIO out, and set to low
    gpio_init( pumpControlPin );
    gpio_set_dir( pumpControlPin, GPIO_OUT );
    gpio_put( pumpAdcPin, 0 );
    m_pumpControlPin = pumpControlPin;

    // Initialise the ADC
    adc_init();
    if( pumpAdcPin == 26 )
    {
        adc_gpio_init( 26 );
        adc_select_input( 0 );
    }
    else if( pumpAdcPin == 27 )
    {
        adc_gpio_init( 27 );
        adc_select_input( 1 );
    }
    else if( pumpAdcPin == 28 )
    {
        adc_gpio_init( 28 );
        adc_select_input( 2 );
    }
    else
    {
        // Not seting up ADC correctly could damage the pump
        for( ;; )
        {
            printf( "ADC setup looks to be incorrect\n" );
            sleep_ms( 2000 );
        }
    }
}
