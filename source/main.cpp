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
// #define TERMINAL_ERROR_COLOUR   ( 0xE005 )
#define TERMINAL_NO_SDC_COLOUR  ( 0xFD44 )
#define TERMINAL_FONT_SIZE      ( 12 )
// If the SD card could not be read or the NTP server could not be connected to,
// the following settings will be used as defaults
#define DEFAULT_WATERING_PERIOD_HOURS   ( 12 )
#define DEFAULT_WATERING_LENGTH_MS      ( 1000 )
// Screen related timings
#define SCREEN_TIMEOUT_MS               ( 5000 )
#define BUTTON_LONG_PRESS_TIME_MS       ( 3000LL )

/* --- MODULE VARIABLES --- */
t_globalData g_globalData;
static char textBuffer[19];

/* --- MODULE SCOPE FUNCTION PROTOTYPE --- */
/* MAIN LOOPS */
static void m_mainLoopNoSdCard( void );
/* REBOOT */
static inline void m_rebootBoard( void );
/* INIT FUNCTIONS */
static inline void m_cyw43Init( void );
static inline void m_oledInit( void );
static inline void m_sdCardInit( void );
/* OTHER FUNCTIONS */
static void m_clearScreen( void );
static void m_timeToString( char* charArrayStart, uint8_t charArraySize, absolute_time_t timestamp );
static void m_terminalLoadingString( char* charArrayStart, uint8_t charArraySize, uint8_t percentageComplete );

// CHANGE THE POWER SAVING MODE WHEN LAUNCHING THE WEB SERVER

int main( void )
{
    // Initialisation
    g_globalData.systemState = e_systemState_init;
    g_globalData.tankState = e_tankState_unknown;
    g_globalData.displayWidth = OLED_DISPLAY_WIDTH;
    g_globalData.displayHeight = OLED_DISPLAY_HEIGHT;

    // Initialise the debug output
    stdio_init_all();
    // printf statements may not work for about a second after calling the stdio init

    // WiFi driver initialisation
    m_cyw43Init();

    // OLED screen initialisation
    m_oledInit();

    // Create a terminal for the display  "MAX TERMINAL WIDTH"
    oled_terminalInit( TERMINAL_FONT_SIZE, TERMINAL_NORMAL_COLOUR );

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
    // if( settings_readFromSDCard( &g_globalData ) != 0 )
    if( true )
    {
        g_globalData.settingsReadOk = false;
        g_globalData.wateringDurationMs = DEFAULT_WATERING_LENGTH_MS;
        oled_terminalSetNewColour( TERMINAL_NO_SDC_COLOUR );
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

    /*
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
    */
    
    printf( "END\n" );

    for( ;; ) {}
}

/* --- MODULE SCOPE FUNCTIONS --- */

/* MAIN LOOPS */
static void m_mainLoopNoSdCard( void )
{
    g_globalData.nextWateringTs = make_timeout_time_ms( ( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL ) / 2UL );
    g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );

    while( true ) // Infinite loop
    {
        /* Need to perform checks in order of importance */

        // If button is pressed and not yet in info mode, print loads of stuff
        if( ( gpio_get( INPUT_BUTTON_PIN ) == true ) &&
            ( g_globalData.systemState != e_systemState_info ) && 
            ( g_globalData.systemState != e_systemState_startWatering ) )
        {
            // Display info
            m_clearScreen();
            g_globalData.systemState = e_systemState_info;
            oled_terminalInit( TERMINAL_FONT_SIZE, TERMINAL_NO_SDC_COLOUR );
            oled_terminalWrite( "NO SD CARD MODE" );
            if( g_globalData.tankState == e_tankState_dry )
            {
                oled_terminalWrite( "TANK IS DRY!" );
            }
            if( g_globalData.tankState == e_tankState_ok )
            {
                oled_terminalWrite( "Tank contains water" );
            }
            else
            {
                oled_terminalWrite( "Tank status unknown" );
            }
            oled_terminalWrite( "Next watering in:" );
            m_timeToString( &textBuffer[0], sizeof( textBuffer ), g_globalData.nextWateringTs );
            oled_terminalWrite( textBuffer );
            oled_terminalWrite( "Hold to water now" );
            oled_terminalSetLine( 6 );
            m_terminalLoadingString( &textBuffer[0], sizeof( textBuffer ), 0 );
            oled_terminalWrite( textBuffer );
            // Set screen timeout
            g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
        }
        // If button pressed but already in info mode, allow for a long press
        if( ( gpio_get( INPUT_BUTTON_PIN ) == true ) &&
            ( g_globalData.systemState == e_systemState_info ) && 
            ( g_globalData.systemState != e_systemState_startWatering ) )
        {
            absolute_time_t buttonPressStartTime = get_absolute_time();
            bool longPress = false;
            int64_t msSinceButtonPressed;
            uint8_t progress;
            while( gpio_get( INPUT_BUTTON_PIN ) )
            {
                // Update the next watering time
                oled_terminalSetLine( 3 );
                m_timeToString( &textBuffer[0], sizeof( textBuffer ), g_globalData.nextWateringTs );
                oled_terminalWrite( textBuffer );
                // Update the long press loading bar thing
                msSinceButtonPressed = absolute_time_diff_us( buttonPressStartTime, get_absolute_time() ) / 1000LL;
                progress = (uint8_t) ( ( msSinceButtonPressed * 100LL ) / BUTTON_LONG_PRESS_TIME_MS );
                oled_terminalSetLine( 6 );
                m_terminalLoadingString( &textBuffer[0], sizeof( textBuffer ), progress );
                oled_terminalWrite( textBuffer );
                if( progress >= 100 )
                {
                    longPress = true;
                    g_globalData.systemState = e_systemState_startWatering;
                    break;
                }
                sleep_ms( 20 );
            }
            if( longPress == false )
            {
                // Set screen timeout
                g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
            }
        }
        // If button not pressed but still in info mode, update the watering time
        // and check if the screen should have timed out
        else if( g_globalData.systemState == e_systemState_info )
        {
            // Check for timeout
            if( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) < 0 )
            {
                // Clear screen due to timeout
                m_clearScreen();
                g_globalData.systemState = e_systemState_idle;
            }
            else
            {
                // Update screen as it has not timed out
                oled_terminalSetLine( 3 );
                m_timeToString( &textBuffer[0], sizeof( textBuffer ), g_globalData.nextWateringTs );
                oled_terminalWrite( textBuffer );
                oled_terminalSetLine( 6 );
                m_terminalLoadingString( &textBuffer[0], sizeof( textBuffer ), 0 );
                oled_terminalWrite( textBuffer );
            }
        }
        // If the system is still on the init screen, check if it should time out
        else if( g_globalData.systemState == e_systemState_init )
        {
            if( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) < 0 )
            {
                g_globalData.systemState = e_systemState_idle;
                m_clearScreen();
            }
        }
        // Check if it's time to water
        else if( ( g_globalData.systemState == e_systemState_startWatering ) || 
                 ( ( absolute_time_diff_us( get_absolute_time(), g_globalData.nextWateringTs ) / 1000LL ) < 0LL ) )
        {
            // Clear screen ready for the pump running screen
            m_clearScreen();
            g_globalData.systemState = e_systemState_startWatering;
            // Run the pump
            pump_run();
            // Set screen state and screen timeout
            g_globalData.systemState = e_systemState_postWatering;
            g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
            // Set the next watering time
            g_globalData.nextWateringTs = make_timeout_time_ms( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL );
        }
        // Check if the post watering screen is being displayed and has timed out
        else if( g_globalData.systemState == e_systemState_postWatering )
        {
            if( ( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) / 1000LL ) < 0LL )
            {
                m_clearScreen();
                g_globalData.systemState = e_systemState_idle;
            }
        }

        sleep_ms( 50 );

        // // Check in order of importance
        // if( g_globalData.flag_displayInfo == true )
        // {
        //     if( g_globalData.state_display != e_displayState_infoTerminal )
        //     {
        //         // Clear the display
        //         m_clearScreen();
        //         // Set new display status
        //         g_globalData.state_display = e_displayState_infoTerminal;
        //         g_globalData.screenChangeTimestamp = get_absolute_time();
        //         // Create a terminal and write the info
        //         oled_terminalInit( 12, TERMINAL_NORMAL_COLOUR );
        //         //                  "MAX TERMINAL WIDTH"
        //         oled_terminalWrite( "No SD card present" );
        //         oled_terminalWrite( "Next watering in:" );
        //         char text[19];
        //         m_timeUntil( &text[0], sizeof(text), g_globalData.nextWateringTimestamp );
        //         oled_terminalWrite( text );
        //     }

        //     // bool longPressActivated = false;
        //     // while( gpio_get( INPUT_BUTTON_PIN ) )
        //     // {

        //     // }

        // }
        // if( g_globalData.flag_watering == true )
        // {
        //     // Clear the display


        //     // Run the pump
        //     pump_run();

        //     // Clear the watering flag
        //     g_globalData.flag_watering = false;
        //     // If a displayInfo flag was raised, it's now not relevant so clear
        //     g_globalData.flag_displayInfo = false;

        //     // Setup a new alarm
        //     cancel_alarm( g_globalData.alarm_watering );
        //     g_globalData.nextWateringTimestamp = make_timeout_time_ms( make_timeout_time_ms( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL ) );
        //     g_globalData.alarm_watering = add_alarm_in_ms( g_globalData.nextWateringTimestamp,
        //                                                    m_noSdCardWateringCb, NULL, false );

        // }
        // else if( g_globalData.flag_displayInfo == true )
        // {
        //     // Display some info
        // }

        // The sleep function should put the board into low power mode for a short time
        // printf( "loop\n" );
        // sleep_ms( 50 );
    }
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

/* OTHER FUNCTIONS */

static void m_clearScreen( void )
{
    if( oled_terminalIsInit() )
    {
        oled_terminalDeinit();
    }
    if( oled_loadingBarIsInit() )
    {
        oled_loadingBarDeinit();
    }
    else if( oled_loadingCircleIsInit() )
    {
        oled_loadingCircleDeinit();
    }

    oled_clear();
}

// Create a string which is HH:MM:SS until the provided time stamp
static void m_timeToString( char* charArrayStart, uint8_t charArraySize, absolute_time_t timestamp )
{
    // Find the time until the timestamp in seconds
    int64_t secondsRemaining = absolute_time_diff_us( get_absolute_time(), timestamp ) / 1000000LL;
    // Calculate the number of hours
    uint8_t hours = (uint8_t) ( secondsRemaining / ( 60LL * 60LL ) );
    secondsRemaining -= (int64_t) hours * ( 60LL * 60LL );
    // Calculate the number of minutes
    uint8_t minutes = (uint8_t) ( secondsRemaining / 60LL );
    secondsRemaining -= (int64_t) minutes * 60LL;
    // Convert to a string
    snprintf( charArrayStart, charArraySize, "%02d:%02d:%02d", hours, minutes, (uint8_t) secondsRemaining );
}

// Create a loading bar string in the form [###    ]
static void m_terminalLoadingString( char* charArrayStart, uint8_t charArraySize, uint8_t percentageComplete )
{
    if( percentageComplete > 100 )
        percentageComplete = 100;
    
    char* charArrayPtr = charArrayStart;
    uint8_t terminalWidth = oled_terminalGetWidthInCharacters();
    uint8_t numberOfHashtags = percentageComplete * ( terminalWidth - 2U ) / 100U;
    for( uint8_t index = 0U; index < terminalWidth; index++ )
    {
        if( index >= charArraySize )
            break;

        if( index == 0U )
        {
            *charArrayPtr = '[';
        }
        else if( index == ( terminalWidth - 1 ) )
        {
            *charArrayPtr = ']';
            break;
        }
        else if( numberOfHashtags > 0 )
        {
            *charArrayPtr = '#';
            --numberOfHashtags;
        }
        else
        {
            *charArrayPtr = ' ';
        }

        ++charArrayPtr;
    }
}
