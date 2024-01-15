#include "sm_init.hpp"

#include "oled.hpp"
#include "pico/cyw43_arch.h"
#include "pump.hpp"
#include "sd_card.h"
#include "ff.h"
#include "settings_reader.hpp"

void smInit_init( t_globalData* globalDataPtr )
{
    // Initialise everything the application needs
    // Initialise parts of the global data struct that haven't been set yet
    globalDataPtr->hardwareData.displayWidth = OLED_DISPLAY_WIDTH;
    globalDataPtr->hardwareData.displayHeight = OLED_DISPLAY_HEIGHT;
    // Initialise the stdio
    stdio_init_all();
    // Initialise the OLED
    if( oled_init( OLED_DATA_IN_PIN, OLED_CLOCK_PIN, OLED_CHIP_SELECT_PIN, 
        OLED_DATA_COMMAND_PIN, OLED_RESET_PIN, OLED_SPI_OUTPUT, OLED_BAUD_RATE_HZ, 
        OLED_DISPLAY_WIDTH, OLED_DISPLAY_HEIGHT ) != 0 )
    {
        for( ;; )
        {
            printf( "oled_init failed\n" );
            sleep_ms( 1000 );
        }
    }
    // Create a terminal with the OLED
    oled_terminalInit( TERMINAL_FONT_12, TERMINAL_INIT_COLOUR );
    oled_terminalWrite( "OLED initialised" );
    // Initialise the WiFi driver, also needed for built-in LED
    oled_terminalWrite( "CYW43 initialising" );
    if( cyw43_arch_init() )
    {
        for( ;; )
        {
            printf( "cyw43_arch_init failed\n" );
            sleep_ms( 1000 );
        }
    }
    // Initialise pump
    oled_terminalWrite( "Initialising pump" );
    pump_init( PUMP_CONTROL_PIN, PUMP_ADC_PIN );
    // Initialise button
    oled_terminalWrite( "Setup button GPIO" );
    gpio_init( INPUT_BUTTON_PIN );
    gpio_set_dir( INPUT_BUTTON_PIN, GPIO_IN );
    // Enable station (STA) mode for the WiFi chip
    oled_terminalWrite( "Enable STA mode" );
    cyw43_arch_enable_sta_mode();
    // Init the SD card driver
    oled_terminalWrite( "Init SDC driver" );
    if( !sd_init_driver() )
    {
        for( ;; )
        {
            printf( "sd_init_driver failed\n" );
            sleep_ms( 1000 );
        }
    }
    // Now attempt to read the SD card
    oled_terminalWrite( "" );
    oled_terminalWrite( "Reading SDC...");
    if( settings_readFromSDCard( globalDataPtr ) != 0 )
    {
        oled_terminalWrite( "Failed to read" );
        oled_terminalWrite( "settings" );
        oled_terminalWrite( "" ); // YOU WERE HERE
    }
    else
    {
        oled_terminalWrite( "Settings read" );
        oled_terminalWrite( "successfully" );
    }
}

void smInit_update( t_globalData* globalDataPtr )
{
    // Check for user input
    // Check if the state has timed out
}