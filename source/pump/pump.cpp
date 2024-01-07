#include "pump.hpp"

#include "stdio.h"
#include "hardware/sync.h"

// If the ADC_THRESHOLD is exceeded then the pump is considered dry
#define ADC_THRESHOLD       ( 2450U )
// Settle time allows the pump to reach a transient speed before adc starts being measured
// the pump may run for this amount of ms before it is detected as being dry
#define PUMP_SETTLE_TIME    ( 500U )

static uint8_t m_pumpControlPin;
static t_globalData* m_globalDataPtr;

void pump_init( t_globalData* globalDataPtr, uint8_t pumpControlPin, uint8_t pumpAdcPin )
{
    // Save the global data pointer
    m_globalDataPtr = globalDataPtr;

    // Initialise the motor control pin as GPIO out, and set to low
    gpio_init( pumpControlPin );
    gpio_set_dir( pumpControlPin, GPIO_OUT );
    gpio_put( pumpAdcPin, 0 );
    m_pumpControlPin = pumpControlPin;

    // Initialise the ADC
    adc_init();
    if( pumpAdcPin == 26 )
    {
        adc_gpio_init( pumpAdcPin );
        adc_select_input( 0 );
    }
    else if( pumpAdcPin == 27 )
    {
        adc_gpio_init( pumpAdcPin );
        adc_select_input( 1 );
    }
    else if( pumpAdcPin == 28 )
    {
        adc_gpio_init( pumpAdcPin );
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

void pump_run( void )
{
    absolute_time_t settleEndTime;
    absolute_time_t pumpEndTime;
    uint16_t adcValue;

    // Calculate end times
    settleEndTime = make_timeout_time_ms( PUMP_SETTLE_TIME );
    pumpEndTime = make_timeout_time_ms( m_globalDataPtr->wateringDurationMs );

    // Start the pump
    gpio_put( m_pumpControlPin, 1 );
    // Wait until the end of the settling time
    while( absolute_time_diff_us( get_absolute_time(), settleEndTime ) > 0 )
    {
        sleep_ms( 10 );
    }
    // Wait until the pump needs to be turned off, or the pump is detected as being dry
    while( absolute_time_diff_us( get_absolute_time(), pumpEndTime ) > 0 )
    {
        // Read the ADC
        adcValue = adc_read();
        
        if( adcValue > ADC_THRESHOLD )
        {
            break;
        }
    }
    // Stop the pump
    gpio_put( m_pumpControlPin, 0 );

}
