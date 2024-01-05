#include <stdio.h>

/* --- PICO SDK LIBRARIES --- */
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"

/* --- FILES FOR THIS PROJECT --- */
#include "oled.hpp"
#include "pump.hpp"
#include "settings.hpp"
#include "global.hpp"

/* --- OTHER INCLUDES --- */

/* --- HARDWARE DEFINITIONS --- */
#define OLED_DATA_IN_PIN        ( 19 )
#define OLED_CLOCK_PIN          ( 18 )
#define OLED_CHIP_SELECT_PIN    ( 17 )
#define OLED_DATA_COMMAND_PIN   ( 16 )
#define OLED_RESET_PIN          ( 20 )
#define OLED_SPI_OUTPUT         ( 0 )
#define OLED_BAUD_RATE_HZ       ( 14000000 ) // 14 MHz
#define OLED_DISPLAY_WIDTH      ( 128 )
#define OLED_DISPLAY_HEIGHT     ( 128 )

#define PUMP_CONTROL_PIN        ( 21 )
#define PUMP_ADC_PIN            ( 26 )

#define INPUT_BUTTON_PIN        ( 2 )

// SD CARD SETTINGS DEFINED IN source/no-OS-FatFS-SD-SPI-Rpi-Pico/FatFs_SPI/sd_driver/hw_config.c

/* --- OTHER DEFINITIONS --- */
// #define TERMINAL_NORMAL_COLOUR  ( 0xFD44 ) // Retro terminal yellow
#define TERMINAL_NORMAL_COLOUR  ( 0x2444 )
#define TERMINAL_ERROR_COLOUR   ( 0xE005 )
// If the SD card could not be read or the NTP server could not be connected to,
// the following settings will be used as defaults
#define DEFAULT_WATERING_PERIOD_HOURS   ( 12 )
#define DEFAULT_WATERING_LENGTH_MS      ( 1000 )

/* --- MODULE SCOPE VARIABLES --- */
t_globalData g_globalData;

/* --- MODULE SCOPE FUNCTION PROTOTYPE --- */
/* MAIN LOOPS */
static void m_mainLoopNoSdCard( void );
/* ALARM CALLBACKS */
static int64_t m_noSdCardWateringCb( alarm_id_t id, void *userData );
/* BUTTON INTERRUPT */
static void m_inputButtonIrqCb( uint gpio, uint32_t events );
/* REBOOT */
static inline void m_rebootBoard( void );
/* INIT FUNCTIONS */
static inline void m_cyw43Init( void );
static inline void m_oledInit( void );
static inline void m_sdCardInit( void );

// CHANGE THE POWER SAVING MODE WHEN LAUNCHING THE WEB SERVER

int main( void )
{
    // Initialisation
    char textBuffer[19];

    // Initialise the debug output
    stdio_init_all();
    // printf statements may not work for about a second after calling the stdio init

    // WiFi driver initialisation
    m_cyw43Init();

    // OLED screen initialisation
    m_oledInit();

    // Create a terminal for the display  "MAX TERMINAL WIDTH"
    oled_terminalInit( 12, TERMINAL_NORMAL_COLOUR );
    g_globalData.displayState = e_displayState_terminal;

    for( uint8_t i = 0U; i < 3; i++ )
        oled_terminalWrite( "" );
    
    oled_terminalWrite( " _           _  __ " );
    oled_terminalWrite( "|_) _. _ o| / \\(_  " );
    oled_terminalWrite( "|_)(_|_> || \\_/__) " );
    oled_terminalWrite( " " );

    // Flicker the built in LED to indicate the pico has booted
    for( int i = 0; i < 3; i++ )
    {
        cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 1 );
        sleep_ms( 50 );
        cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 0 );
        sleep_ms( 50 );
    }

    oled_terminalWrite( "OLED initialised" );
    oled_terminalWrite( "CYW43 initialised" );

    // Initialise the pump
    oled_terminalWrite( "Initialising pump" );
    // ! The pump init function will need rewriting if other ADC is used !
    pump_init( &g_globalData, PUMP_CONTROL_PIN, PUMP_ADC_PIN );

    // Intialise input button
    oled_terminalWrite( "Init input button" );
    gpio_init( INPUT_BUTTON_PIN );
    gpio_set_dir( INPUT_BUTTON_PIN, GPIO_IN );

    // Enable station mode for the WiFi driver
    oled_terminalWrite( "Enable STA mode" );
    cyw43_arch_enable_sta_mode();

    // SD card driver init
    oled_terminalWrite( "Init SDC driver" );
    m_sdCardInit();

    // Read the settings.txt file from the SD card
    oled_terminalWrite( "Load SDC settings" );
    if( settings_readFromSDCard( &g_globalData ) != 0 )
    {
        g_globalData.settingsReadOk = false;
        g_globalData.wateringDurationMs = DEFAULT_WATERING_LENGTH_MS;
        oled_terminalSetNewColour( TERMINAL_ERROR_COLOUR );
        oled_terminalWrite( "ERROR Cannot read" );
        oled_terminalWrite( "      settings.txt" );
        sleep_ms( 1000 );
        oled_terminalWrite( "Will use default" );
        oled_terminalWrite( "settings:" );
        oled_terminalWrite( "Watering every" );
        snprintf( textBuffer, sizeof(textBuffer), "%d hours for %d", DEFAULT_WATERING_PERIOD_HOURS, DEFAULT_WATERING_LENGTH_MS );
        oled_terminalWrite( textBuffer );
        oled_terminalWrite( "milliseconds" );
        oled_terminalWrite( "" );

        m_mainLoopNoSdCard(); // This function will never exit
    }
    
    g_globalData.settingsReadOk = true;

    // // Show off by reading some of the SD card settings
    // snprintf( textBuffer, sizeof(textBuffer), "->SSID=%s", g_globalData.wifiSsid );
    // oled_terminalWrite( textBuffer );
    // uint8_t wateringTimes = 0U;
    // for( uint8_t index = 0; index < MAX_NUMBER_OF_WATERING_TIMES; index++ )
    // {
    //     if( g_globalData.wateringTimes[index] != -1 )
    //         ++wateringTimes;
    // }
    // snprintf( textBuffer, sizeof(textBuffer), "->Water %d times", wateringTimes );
    // oled_terminalWrite( textBuffer );
    // oled_terminalWrite( "  per day" );
    // snprintf( textBuffer, sizeof(textBuffer), "->Water for %d", g_globalData.wateringDurationMs );
    // oled_terminalWrite( textBuffer );
    // oled_terminalWrite( "  milliseconds" );



    // if( settings_readFromSDCard( &g_globalData ) == 0 )
    // {
    //     m_settings.settingsReadOk = true;
    //     oled_terminalWrite( "Read complete!" );
    //     // SHOW OFF BY WRITING SOME OF THE SETTINGS
    // }
    // else
    // {
    //     m_settings.settingsReadOk = false;
    //     oled_terminalWrite( "Read failed" );
    // }

    // // Connect to WiFi
    // oled_terminalWrite( "Connecting to SSID" );
    // snprintf( textBuffer, sizeof(textBuffer), "\"%s\"", WIFI_SSID );
    // oled_terminalWrite( textBuffer );

    // cyw43_arch_wifi_connect_timeout_ms( WIFI_SSID,
    //     WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000 );

    

    printf( "END\n" );

    for( ;; ) {}
}

/* --- MODULE SCOPE FUNCTIONS --- */
/* MAIN LOOPS */
static void m_mainLoopNoSdCard( void )
{
    // Set an alarm for watering the plant
    g_globalData.alarmIdWatering = add_alarm_in_ms( ( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL ) / 2UL, 
                                                    m_noSdCardWateringCb, NULL, false );

    // Set a interrupt on the input button pin
    gpio_set_irq_enabled_with_callback( INPUT_BUTTON_PIN,
                                        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                        true,
                                        &m_inputButtonIrqCb );

    while( true ) // Infinite loop
    {
        // Sleep_ms should automatically put the board into low power mode where possible
        sleep_ms( 0xFFFFFFFFU ); // Maximum sleep
    }
}
/* ALARM CALLBACKS */
static int64_t m_noSdCardWateringCb( alarm_id_t id, void *userData )
{
    // Run the pump, timing is based on global data struct
    pump_run();
    // Cancel the current watering timer
    cancel_alarm( g_globalData.alarmIdWatering );
    // Set a new alarm
    g_globalData.alarmIdWatering = add_alarm_in_ms( ( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL ), 
                                                    m_noSdCardWateringCb, NULL, false );
    
    return 0;
}

// static void m_taskClearDisplay( void )
// {
//     switch( g_globalData.displayState )
//     {
//         case e_displayState_terminal:
//         {

//         }
//         break;
//         case e_displayState_image:
//         {

//         }
//         break;
//         case e_displayState_clear:
//         default:
//         {
//             // Do nothing
//         }
//     }

//     g_globalData.displayState = e_displayState_clear;
// }

/* BUTTON INTERRUPT */
static void m_inputButtonIrqCb( uint gpio, uint32_t events )
{
    // TODO
}

/* REBOOT */
static inline void m_rebootBoard( void )
{
    // Reboot the board by enabling a 1ms watchdog, and then not feeding it
    watchdog_enable( 1, false );
    for( ;; ) {}
}
/* INIT FUNCTIONS */
static inline void m_cyw43Init( void )
{
    if( cyw43_arch_init() )
    {
        for( ;; )
        {
            printf( "cyw43_arch_init failed\n" );
            sleep_ms( 1000 );
        }
    }
}

static inline void m_oledInit( void )
{
    if( oled_init( OLED_DATA_IN_PIN, OLED_CLOCK_PIN, OLED_CHIP_SELECT_PIN, 
        OLED_DATA_COMMAND_PIN, OLED_RESET_PIN, OLED_SPI_OUTPUT, OLED_BAUD_RATE_HZ, 
        OLED_DISPLAY_WIDTH, OLED_DISPLAY_HEIGHT ) != 0 )
    {
        cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 1 );
        for( ;; )
        {
            printf( "oled_init failed\n" );
            sleep_ms( 1000 );
        }
    }
}

static inline void m_sdCardInit( void )
{
    if( !sd_init_driver() )
    {
        cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 1 );
        for( ;; )
        {
            printf( "sd_init_driver failed\n" );
            sleep_ms( 1000 );
        }
    }
}
