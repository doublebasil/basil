#include "pump.hpp"

#include "stdio.h"
#include "hardware/sync.h"

#include "intcos.hpp"
#include "oled.hpp"

// If the ADC_THRESHOLD is exceeded then the pump is considered dry
#define ADC_THRESHOLD       ( 2450U )
// Settle time allows the pump to reach a transient speed before adc starts being measured
// the pump may run for this amount of ms before it is detected as being dry
#define PUMP_SETTLE_TIME_MS ( 500U )

#define GAUGE_INNER_RADIUS      ( 40 )
#define GAUGE_OUTER_RADIUS      ( 50 )
#define GAUGE_REDLINE_THICKNESS ( 1 )
#define GAUGE_REDLINE_LENGTH    ( 60 )
#define GAUGE_COLOUR            ( 0xFFFF )
#define GAUGE_REDLINE_COLOUR    ( 0xFF00 )

static uint8_t m_pumpControlPin;
static t_globalData* m_globalDataPtr;

static void m_drawRedline( uint16_t redlinePosition );

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
    printf("pump_run\n");

    absolute_time_t settleEndTime;
    absolute_time_t pumpEndTime;
    uint16_t adcValue;

    // Draw the redline
    m_drawRedline( ADC_THRESHOLD );

    // Calculate end times
    settleEndTime = make_timeout_time_ms( PUMP_SETTLE_TIME_MS );
    pumpEndTime = make_timeout_time_ms( m_globalDataPtr->wateringDurationMs );

    // Start the pump
    gpio_put( m_pumpControlPin, 1 );
    // Wait until the end of the settling time
    while( absolute_time_diff_us( get_absolute_time(), settleEndTime ) > 0 )
    {
        sleep_ms( 100 );
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

    printf("pump exit\n");
}

// Draw the redline for the loading circle which shows where the dry detection cutoff is
static void m_drawRedline( uint16_t redlinePosition )
{
    uint8_t displayCenterX = m_globalDataPtr->displayWidth / 2;
    uint8_t displayCenterY = m_globalDataPtr->displayHeight / 2;

    if( redlinePosition > 0x0FFF )
        redlinePosition = 0x0FFF;

    int16_t theta = (int16_t) ( ( (uint64_t) redlinePosition * 360ULL ) / 0x0FFFULL );

    if( theta < 90 )
    {
        oled_drawLineBetweenPoints( displayCenterX, 
                                    displayCenterY,
                                    displayCenterX + ( ( GAUGE_REDLINE_LENGTH * intsin( theta ) ) / 1000U ),
                                    displayCenterY - ( ( GAUGE_REDLINE_LENGTH * intcos( theta ) ) / 1000U ),
                                    GAUGE_REDLINE_COLOUR,
                                    GAUGE_REDLINE_THICKNESS );
    }
    else if( theta < 180 ) // and theta >= 90
    {
        oled_drawLineBetweenPoints( displayCenterX, 
                                    displayCenterY,
                                    displayCenterX + ( ( GAUGE_REDLINE_LENGTH * intcos( theta - 90 ) ) / 1000U ),
                                    displayCenterY + ( ( GAUGE_REDLINE_LENGTH * intsin( theta - 90 ) ) / 1000U ),
                                    GAUGE_REDLINE_COLOUR,
                                    GAUGE_REDLINE_THICKNESS );
    }
    else if( theta < 270 ) // and theta >- 180
    {
        oled_drawLineBetweenPoints( displayCenterX, 
                                    displayCenterY,
                                    displayCenterX - ( ( GAUGE_REDLINE_LENGTH * intsin( theta - 180 ) ) / 1000U ),
                                    displayCenterY + ( ( GAUGE_REDLINE_LENGTH * intcos( theta - 180 ) ) / 1000U ),
                                    GAUGE_REDLINE_COLOUR,
                                    GAUGE_REDLINE_THICKNESS );
    }
    else // 270 <= theta < 360
    {
        oled_drawLineBetweenPoints( displayCenterX, 
                                    displayCenterY,
                                    displayCenterX - ( ( GAUGE_REDLINE_LENGTH * intcos( theta - 270 ) ) / 1000U ),
                                    displayCenterY - ( ( GAUGE_REDLINE_LENGTH * intsin( theta - 270 ) ) / 1000U ),
                                    GAUGE_REDLINE_COLOUR,
                                    GAUGE_REDLINE_THICKNESS );
    }
}
