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

// Check that only the necessary parts of the oled code have been included. This saves several kB of RAM
// Parts of oled.hpp that should have been included
#if !defined(OLED_INCLUDE_FONT12) || !defined(OLED_INCLUDE_FONT20)  || !defined(OLED_INCLUDE_LOADING_CIRCLE) || !defined(OLED_INCLUDE_LOADING_BAR_HORIZONTAL) || !defined(OLED_INCLUDE_SD_IMAGES) || !defined(OLED_INCLUDE_QR_GENERATOR)
#error "A required part of oled.hpp has not been included"
// Parts of oled.hpp that should not have been included
#elif defined(OLED_INCLUDE_FONT8) || defined(OLED_INCLUDE_FONT16) || defined(OLED_INCLUDE_FONT24) || defined(OLED_INCLUDE_TEST_FUNCTION)
#error "An unnecessary part of oled.hpp has been included, this can waste lots of RAM or program space"
#endif

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
#define TERMINAL_NORMAL_COLOUR          ( 0x2444 ) // Forest green
#define TERMINAL_ERROR_COLOUR           ( 0b0000000000011111 ) // Blue
#define TERMINAL_FONT_SIZE              ( 12 )
#define RGB565_YELLOW                   ( 0xFFE0 )
// If the SD card could not be read or the NTP server could not be connected to,
// the following settings will be used as defaults
#define DEFAULT_WATERING_PERIOD_HOURS   ( 12 )
#define DEFAULT_WATERING_LENGTH_MS      ( 1000 )
/* TIMING RELATED MAGIC NUMBERS */
#define MAIN_LOOP_SLEEP_DURATION_MS     ( 50 )
#define SCREEN_TIMEOUT_MS               ( 15000 )
#define BUTTON_LONG_PRESS_TIME_MS       ( 3000LL )
// When the idle screen notification icon is show, the following timings are used
#define NOTIFICATION_ON_TIME            ( 1000 )
#define NOTIFICATION_OFF_TIME           ( 4000 )
#define INFO_TIME_PER_SCREEN_MS         ( 7500 )
#define INFO_MIN_SCREEN_TIME_MS         ( 3000 ) // Don't swap to a new screen if the info state will time out within this time

#define DEBUG_PRINT_STATE_CHANGES

// #define PRINTDEBUG(text, ...) printf(text, __VA_ARGS__) // Comment this line to disable
// #ifndef PRINTDEBUG
// #define PRINTDEBUG(text, ...) ( (void) 0 )
// #endif

/* --- MODULE VARIABLES --- */
t_globalData g_globalData;
static char m_textBuffer[19];

/* --- MODULE SCOPE FUNCTION PROTOTYPE --- */
/* MAIN LOOP FUNCTIONS */
static inline void m_loopInit( void );
static inline void m_loopIdle( void );
static inline void m_loopInfo( void );
/* REBOOT */
static inline void m_rebootBoard( void );
/* INIT FUNCTIONS */
static inline void m_cyw43Init( void );
static inline void m_oledInit( void );
static inline void m_sdCardInit( void );
/* OTHER FUNCTIONS */
static void m_basilOsScreen( void );
static bool m_checkRecyclingBinday( void );
static bool m_checkLandfillBinday( void );
static void inline m_setSystemState( t_systemState state );
static void m_clearScreen( void );
static void m_timeToString( char* charArrayStart, uint8_t charArraySize, absolute_time_t timestamp );
static void m_terminalLoadingBar( char* charArrayStart, uint8_t charArraySize, uint8_t percentageComplete );
static inline bool m_checkRepeatedButtonPress( void );
static inline bool m_checkDoubleButtonPress( void );
static inline void m_infoStateMoveToNextScreen( void );

// CHANGE THE POWER SAVING MODE WHEN LAUNCHING THE WEB SERVER

int main( void )
{
    // Initialisation
    g_globalData.systemState = e_systemState_init;
    g_globalData.tankState = e_tankState_unknown;
    g_globalData.displayWidth = OLED_DISPLAY_WIDTH;
    g_globalData.displayHeight = OLED_DISPLAY_HEIGHT;
    g_globalData.buttonPin = INPUT_BUTTON_PIN;
    g_globalData.doublePressRegisteredTimestamp = nil_time;
    g_globalData.repeatedButtonPressRegisteredTimestamp = nil_time;
    for( uint8_t index = 0U; index < INFO_SCREEN_REPEATED_PRESSES; index++ )
        g_globalData.buttonPressTimestamps[index] = nil_time;

    // Initialise the debug output
    stdio_init_all();
    // printf statements may not work for about a second after calling the stdio init

    // WiFi driver initialisation, also needed for on built-in LED
    m_cyw43Init();

    // OLED screen initialisation
    m_oledInit();

    // Create a terminal for the display  "MAX TERMINAL WIDTH"
    oled_terminalInit( TERMINAL_FONT_SIZE, TERMINAL_NORMAL_COLOUR );

    // Run the BASIL OS screen, which also flickers the built in LED
    m_basilOsScreen();

    oled_terminalWrite( "OLED initialised" );
    oled_terminalWrite( "CYW43 initialised" );

    // Initialise the pump
    oled_terminalWrite( "Initialising pump" );
    // ! The pump init function will need rewriting if other ADC is used !
    pump_init( &g_globalData, PUMP_CONTROL_PIN, PUMP_ADC_PIN );

    // Intialise input button
    oled_terminalWrite( "Init input button" );
    gpio_init( g_globalData.buttonPin );
    gpio_set_dir( g_globalData.buttonPin, GPIO_IN );

    // Enable station mode for the WiFi driver
    oled_terminalWrite( "Enable STA mode" );
    cyw43_arch_enable_sta_mode();

    // SD card driver init
    oled_terminalWrite( "Init SDC driver" );
    m_sdCardInit();

    // Read the settings.txt file from the SD card
    oled_terminalWrite( "Load SDC settings" );
    // if( settings_readFromSDCard( &g_globalData ) != 0 )
    sleep_ms( 300 ); if( true )
    {
        // Failed to read SD card, update the relevant global data
        g_globalData.settingsReadOk = false;
        g_globalData.wateringDurationMs = DEFAULT_WATERING_LENGTH_MS;

        // Display message to inform user
        oled_terminalClear();
        sleep_ms( 500 );
        oled_terminalWrite( "Error:" );
        oled_terminalWrite( "Couldn't read from" );
        oled_terminalWrite( "SD card" );
        oled_terminalWrite( " " );
        oled_terminalWrite( "Continuing with" );
        oled_terminalWrite( "default settings" );
        oled_terminalWrite( " " );
        snprintf( m_textBuffer, sizeof( m_textBuffer ) , "Water for %d ms", g_globalData.wateringDurationMs );
        oled_terminalWrite( m_textBuffer );
        snprintf( m_textBuffer, sizeof( m_textBuffer ) , "Every %d hours", DEFAULT_WATERING_PERIOD_HOURS );
        oled_terminalWrite( m_textBuffer );

        // Update global data timestamps before main loop
        g_globalData.nextWateringTs = make_timeout_time_ms( ( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL ) / 2UL );
        g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
    }
    else
    {
        g_globalData.settingsReadOk = true;
        oled_terminalDeinit();
        oled_clear();
        // Do some fancy shtuff with icons n that
        // TODO
        for( ;; ) sleep_ms( 1000 );
    }

    // TESTING
    g_globalData.tankState = e_tankState_dry;

    // Main program loop
    while( true )
    {
        switch( g_globalData.systemState )
        {
            case e_systemState_init:
            {
                m_loopInit();
            }
            break;
            case e_systemState_idle:
            {
                m_loopIdle();
            }
            break;
            case e_systemState_info:
            {
                m_loopInfo();
            }
            break;
            case e_systemState_watering:
            {
                // TODO
            }
            break;
            default:
            {
                // Do nothing
            }
            break;
        }
        sleep_ms( MAIN_LOOP_SLEEP_DURATION_MS );
    }

    // If the loop somehow exits, reboot the board
    m_rebootBoard();
    
}

/* --- MODULE SCOPE FUNCTIONS --- */

/* MAIN LOOP FUNCTIONS */
static inline void m_loopInit( void )
{
    // If the button is pressed, go to info state
    if( gpio_get( g_globalData.buttonPin ) )
    {
        m_clearScreen();
        m_setSystemState( e_systemState_info );
    }
    // Else, check if the screen has timed out
    else if( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) < 0 )
    {
        // Screen has timed out, so go into idle state
        m_clearScreen();
        m_setSystemState( e_systemState_idle );
    }
}

static inline void m_loopIdle( void )
{
    if( g_globalData.stateHasInitialised == false )
    {
        g_globalData.notificationDisplayed = false;
        g_globalData.stateHasInitialised = true;
    }

    // Check for button input
    if( gpio_get( g_globalData.buttonPin ) )
    {
        // The button is pressed so go into the info state
        m_clearScreen();
        m_setSystemState( e_systemState_info );
    }
    // Check if it's time to water
    else if( absolute_time_diff_us( get_absolute_time(), g_globalData.nextWateringTs ) < 0 )
    {
        m_clearScreen();
        m_setSystemState( e_systemState_watering );
    }
    // If there is no user input, check for something to notify the user of
    else if( g_globalData.tankState == e_tankState_dry ||
             m_checkRecyclingBinday() || 
             m_checkLandfillBinday() )
    {
        // Display an intermittent notification screen
        if( ( to_us_since_boot( get_absolute_time() ) / 1000ULL ) % ( NOTIFICATION_OFF_TIME + NOTIFICATION_ON_TIME ) > NOTIFICATION_OFF_TIME )
        {
            // Notification on
            if( ( g_globalData.settingsReadOk == false )
                && ( g_globalData.notificationDisplayed == false ) )
            {
                // No SD card available, so use a terminal
                oled_terminalInit( 12, RGB565_YELLOW );
                oled_terminalWrite( "" );
                oled_terminalWrite( "        ##" );
                oled_terminalWrite( "     ########" );
                oled_terminalWrite( "    #        #" );
                oled_terminalWrite( "   #          #" );
                oled_terminalWrite( "   #          #" );
                oled_terminalWrite( "   #          #" );
                oled_terminalWrite( "   ############" );
                oled_terminalWrite( "  #     /\\     #" );
                oled_terminalWrite( "   #####\\/#####" );
                g_globalData.notificationDisplayed = true;
            }
            else
            {
                // TODO
            }
        }
        else if( g_globalData.notificationDisplayed == true )
        {
            // Blank screen, prevent OLED burn-in
            // m_clearScreen will automatically deinit a terminal if it was created
            m_clearScreen();
            g_globalData.notificationDisplayed = false;
        }
    }
}

static inline void m_loopInfo( void )
{
    // Initialise the info state if not already initialised
    if( g_globalData.stateHasInitialised == false )
    {
        // Determine which screen to show first
        // Anything that causes the notification icon to show will get priority here
        if( g_globalData.tankState == e_tankState_dry )
            g_globalData.infoMode = e_infoScreen_dryTank;
        else if( m_checkRecyclingBinday() )
            g_globalData.infoMode = e_infoScreen_binDayRecycling;
        else if( m_checkLandfillBinday() )
            g_globalData.infoMode = e_infoScreen_binDayLandfill;
        else
            g_globalData.infoMode = e_infoScreen_standard;
        
        g_globalData.showHelpScreen = false;
        g_globalData.showLongPressScreen = false;
        
        // Wait for button to be released
        while( gpio_get( g_globalData.buttonPin ) )
            sleep_ms( 50 );
        
        g_globalData.displayedInfo = e_infoScreen_none;
        g_globalData.buttonPressTsIndex = 0U;
        g_globalData.stateHasInitialised = true;
        // Set the screen timeout, which is when this state will end
        g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
    }
    // Check if the state has timed out
    if( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) < 0 )
    {
        // Go into idle mode
        m_clearScreen();
        m_setSystemState( e_systemState_idle );
    }
    // Check for button presses
    if( gpio_get( g_globalData.buttonPin ) )
    {
        // Log the button press
        absolute_time_t buttonPressTime = get_absolute_time();
        g_globalData.buttonPressTimestamps[g_globalData.buttonPressTsIndex] = buttonPressTime;
        ++g_globalData.buttonPressTsIndex;
        if( g_globalData.buttonPressTsIndex == INFO_SCREEN_REPEATED_PRESSES )
            g_globalData.buttonPressTsIndex = 0U;
        
        bool longPressActivated = false;
        absolute_time_t longPressStartTime = get_absolute_time();

        // Check if the button is being held
        while( gpio_get( g_globalData.buttonPin ) )
        {
            // TODO
            if( ( absolute_time_diff_us( longPressStartTime, get_absolute_time() ) > ( LONG_PRESS_MIN_TIME_MS * 1000LL ) ) 
                && ( longPressActivated == false ) )
            {
                longPressActivated = true;
                printf("Long press\n");
                // Initialise the long press screen
                // TODO
            }
            else if( longPressActivated == true )
            {
                // Update the long press screen
                // TODO
            }

            sleep_ms( 50 );
        }

        if( longPressActivated == false )
        {
            // If a long press was not detected, process the 
            if( m_checkRepeatedButtonPress() )
            {
                // Process a repeated button press
                // Set to propapropapropaganganda mode
                g_globalData.infoMode = e_infoScreen_propaganda;
                g_globalData.showHelpScreen = false;
                g_globalData.showLongPressScreen = false;
                g_globalData.altScreenTimeoutTimestamp = nil_time;
            }
            else if( m_checkDoubleButtonPress() )
            {
                // Process a double button press
                // Do not show the help screen
                g_globalData.showHelpScreen = false;
                g_globalData.showLongPressScreen = false;
                g_globalData.altScreenTimeoutTimestamp = nil_time;
                // Move to the next screen
                m_infoStateMoveToNextScreen();
            }
            else
            {
                // Process a single button press
                // Show help if not already shown, hide help if already shown
                if( g_globalData.showHelpScreen == false )
                {
                    g_globalData.showHelpScreen = true;
                    g_globalData.altScreenTimeoutTimestamp = make_timeout_time_ms( HELP_SCREEN_TIMEOUT_MS );
                }
                else
                {
                    g_globalData.showHelpScreen = false;
                    g_globalData.altScreenTimeoutTimestamp = nil_time; // Don't timeout
                }
            }
        }

        // Update the timeout time
        g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
    }
    // Check if the alt screen has timed out
    if( ( is_nil_time( g_globalData.altScreenTimeoutTimestamp ) == false )
        && ( absolute_time_diff_us( get_absolute_time(), g_globalData.altScreenTimeoutTimestamp ) < 0LL ) )
    {
        g_globalData.showHelpScreen = false;
        g_globalData.showLongPressScreen = false;
        g_globalData.altScreenTimeoutTimestamp = nil_time;
        g_globalData.displayedInfo = e_infoScreen_none;
    }

    // Display stuff
    if( g_globalData.showLongPressScreen == true )
    {
        // Leave the display as it was set earlier by the long press function
    }
    else if( g_globalData.infoMode == e_infoScreen_dryTank )
    {
        // Check if the help screen should be shown
        if( g_globalData.showHelpScreen == true )
        {
            m_clearScreen();
            if( g_globalData.settingsReadOk == false )
            {
                // No SD card help
                oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
                oled_terminalWrite( "<tank state>" );
                oled_terminalWrite( "<help>" );
                oled_terminalWrite( "Single press:" );
                oled_terminalWrite( "- Show help" );
                oled_terminalWrite( "Double press:" );
                oled_terminalWrite( "- Next info" );
                oled_terminalWrite( "Hold:" );
                oled_terminalWrite( "- Dismiss notification" );
            }
            else
            {
                // Even if icons are available, help will be very similar
                oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
                oled_terminalWrite( "<help>" );
                oled_terminalWrite( "Single press:" );
                oled_terminalWrite( "- Show help" );
                oled_terminalWrite( "Double press:" );
                oled_terminalWrite( "- Next info" );
                oled_terminalWrite( "Hold:" );
                oled_terminalWrite( "- Dismiss notification" );
            }
        }
        // Check if this is the first time this screen is running
        else if( g_globalData.infoMode != g_globalData.displayedInfo )
        {
            m_clearScreen();
            if( g_globalData.settingsReadOk == false )
            {
                // Terminal version
                oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
                oled_terminalWrite( "<tank state>" );
                oled_terminalWrite( "" );
                oled_terminalWrite( "Dry tank" );
                oled_terminalWrite( "detected" );
            }
            else
            {
                // Icon version
                // TODO
            }
            g_globalData.displayedInfo = g_globalData.infoMode;
        }
    }
    else if( g_globalData.infoMode == e_infoScreen_binDayLandfill )
    {
        if( g_globalData.settingsReadOk == false )
        {
            // Do nothing, bin day checking is only possible with the SD card present
        }
        // Check if the help screen should be shown
        else if( g_globalData.showHelpScreen == true )
        {
            m_clearScreen();
            oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
            oled_terminalWrite( "<help>" );
            oled_terminalWrite( "Single press:" );
            oled_terminalWrite( "- Show help" );
            oled_terminalWrite( "Double press:" );
            oled_terminalWrite( "- Next info" );
            oled_terminalWrite( "Hold:" );
            oled_terminalWrite( "- Dismiss notification" );
        }
        // Check if this is the first time this screen is running
        else if( g_globalData.infoMode != g_globalData.displayedInfo )
        {
            // TODO
            // Show the bin day icon
            g_globalData.displayedInfo = g_globalData.infoMode;
        }
    }
    else if( g_globalData.infoMode == e_infoScreen_binDayRecycling )
    {
        if( g_globalData.settingsReadOk == false )
        {
            // Do nothing, bin day checking is only possible with the SD card present
        }
        // Check if the help screen should be shown
        else if( g_globalData.showHelpScreen == true )
        {
            m_clearScreen();
            oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
            oled_terminalWrite( "<help>" );
            oled_terminalWrite( "Single press:" );
            oled_terminalWrite( "- Show help" );
            oled_terminalWrite( "Double press:" );
            oled_terminalWrite( "- Next info" );
            oled_terminalWrite( "Hold:" );
            oled_terminalWrite( "- Dismiss notification" );
        }
        // Check if this is the first time this screen is running
        else if( g_globalData.infoMode != g_globalData.displayedInfo )
        {
            // TODO
            // Show the bin day icon
            g_globalData.displayedInfo = g_globalData.infoMode;
        }
    }
    else if( g_globalData.infoMode == e_infoScreen_standard )
    {
        // Check if the help screen should be shown
        if( g_globalData.showHelpScreen == true )
        {
            m_clearScreen();
            if( g_globalData.settingsReadOk == false )
            {
                // No SD card help
                oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
                oled_terminalWrite( "<info screen>" );
                oled_terminalWrite( "<help>" );
                oled_terminalWrite( "Single press:" );
                oled_terminalWrite( "- Show help" );
                oled_terminalWrite( "Double press:" );
                oled_terminalWrite( "- Next info" );
                oled_terminalWrite( "Hold:" );
                oled_terminalWrite( "- Water now" );
            }
            else
            {
                // Even if icons are available, help will be very similar
                oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
                oled_terminalWrite( "<help screen>" );
                oled_terminalWrite( "Single press:" );
                oled_terminalWrite( "- Show help" );
                oled_terminalWrite( "Double press:" );
                oled_terminalWrite( "- Next info" );
                oled_terminalWrite( "Hold:" );
                oled_terminalWrite( "- Water now" );
            }
        }
        // Check if this is the first time this screen is running
        else if( g_globalData.infoMode != g_globalData.displayedInfo )
        {
            m_clearScreen();
            if( g_globalData.settingsReadOk == false )
            {
                // Terminal version
                oled_terminalInit( 20, TERMINAL_ERROR_COLOUR );
                oled_terminalWrite( "<info screen>" );
                oled_terminalWrite( "" );
                oled_terminalWrite( "Next watering:" );
                m_timeToString( m_textBuffer, sizeof( m_textBuffer ), g_globalData.nextWateringTs );
                oled_terminalWrite( m_textBuffer ); // LINE NO. 3
            }
            else
            {
                // Icon version
                // TODO
            }
            g_globalData.displayedInfo = g_globalData.infoMode;
        }
        else
        {
            if( g_globalData.settingsReadOk == false )
            {
                // Update the timing on the screen
                oled_terminalSetLine( 3 );
                m_timeToString( m_textBuffer, sizeof( m_textBuffer ), g_globalData.nextWateringTs );
                oled_terminalWrite( m_textBuffer );
            }
            else
            {
                // TODO 
                // Update the time in the icon version
            }
        }
    }
    else if( g_globalData.infoMode == e_infoScreen_propaganda )
    {
        if( g_globalData.settingsReadOk == false )
        {
            // In terminal mode there's only so much propaganda I can create
            const uint32_t numberOfPhrases = 5;
            oled_terminalInit( 12, 0b1111100000000000 );
            oled_terminalWrite( "Err..." );
            switch( to_ms_since_boot( get_absolute_time() ) % numberOfPhrases )
            {
                case 0:
                {
                    oled_terminalWrite("Friends");
                    oled_terminalWrite("Not Food!");
                }
                break;
                case 1:
                {
                    oled_terminalWrite("Meat is");
                    oled_terminalWrite("Murder!");
                }
                break;
                case 2:
                {
                    oled_terminalWrite("Huggers");
                    oled_terminalWrite("Not Hunters!");
                }
                break;
                case 3:
                {
                    oled_terminalWrite("My Body Is");
                    oled_terminalWrite("Not A Graveyard!");
                }
                break;
                case 4:
                default:
                {
                    oled_terminalWrite("Yes Ve gan");
                    oled_terminalWrite("(Wtf am I doing)");
                }
                break;
            }
            oled_terminalWrite( "" );
            oled_terminalWrite( "(I need the SD" );
            oled_terminalWrite( " card for the" );
            oled_terminalWrite( " real propaganda!)" );
        }
        else
        {
            // Go to town with some toxic (but not vulgar!) propaganda images
            // TODO
        }
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

static void m_basilOsScreen( void )
{
    bool ledState = true;
    char loadingChars[4] = {'|', '/', '-', '\\'};

    for( uint8_t i = 0U; i < 3; i++ )
        oled_terminalWrite( "" );

    oled_terminalWrite( " _           _  __" );
    oled_terminalWrite( "|_) _. _ o| / \\(_" );
    oled_terminalWrite( "|_)(_|_> || \\_/__)" );
    oled_terminalWrite( " " );

    for( uint8_t loops = 0U; loops < 4; loops++ )
    {
        for( uint8_t characterIndex = 0U; characterIndex < 4; characterIndex++ )
        {
            snprintf( m_textBuffer, sizeof( m_textBuffer ), "        %c", loadingChars[characterIndex] );
            oled_terminalWriteTemp( m_textBuffer );
            cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, ledState );
            ledState = !ledState;
            sleep_ms( 100 );
        }
    }
    cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 0 );
}

// Return true if binday is soon
static bool m_checkRecyclingBinday( void )
{
    if( g_globalData.settingsReadOk == false )
        return false;
    else
    {
        // TODO
        return false;
    }
}

// Return true if binday is soon
static bool m_checkLandfillBinday( void )
{
    if( g_globalData.settingsReadOk == false )
        return false;
    else
    {
        // TODO
        return false;
    }
}

static inline void m_setSystemState( t_systemState state )
{
#ifdef DEBUG_PRINT_STATE_CHANGES
    char from[10];
    char to[10];
    switch( g_globalData.systemState )
    {
        case e_systemState_init:
        {
            snprintf( from, sizeof( from ), "init" );
        }
        break;
        case e_systemState_idle:
        {
            snprintf( from, sizeof( from ), "idle" );
        }
        break;
        case e_systemState_info:
        {
            snprintf( from, sizeof( from ), "info" );
        }
        break;
        case e_systemState_watering:
        {
            snprintf( from, sizeof( from ), "watering" );
        }
        break;
        default:
        {
            snprintf( from, sizeof( from ), "unknown" );
        }
        break;
    }
    switch( state )
    {
        case e_systemState_init:
        {
            snprintf( to, sizeof( to ), "init" );
        }
        break;
        case e_systemState_idle:
        {
            snprintf( to, sizeof( to ), "idle" );
        }
        break;
        case e_systemState_info:
        {
            snprintf( to, sizeof( to ), "info" );
        }
        break;
        case e_systemState_watering:
        {
            snprintf( to, sizeof( to ), "watering" );
        }
        break;
        default:
        {
            snprintf( to, sizeof( to ), "unknown" );
        }
        break;
    }
    printf( "State change %s -> %s\n", from, to );
#endif
    g_globalData.stateHasInitialised = false;
    g_globalData.systemState = state;
    // g_globalData.stateStartTimestamp = get_absolute_time();
}

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
static void m_terminalLoadingBar( char* charArrayStart, uint8_t charArraySize, uint8_t percentageComplete )
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

static inline bool m_checkRepeatedButtonPress( void )
{
    int16_t mostRecentButtonPressIndex = (int16_t) g_globalData.buttonPressTsIndex - 1;
    if( mostRecentButtonPressIndex < 0 )
        mostRecentButtonPressIndex += INFO_SCREEN_REPEATED_PRESSES;
    
    int16_t oldestButtonPressIndex = (int16_t) g_globalData.buttonPressTsIndex;
    if( oldestButtonPressIndex < 0 )
        oldestButtonPressIndex += INFO_SCREEN_REPEATED_PRESSES;

    absolute_time_t mostRecentButtonPressTs = g_globalData.buttonPressTimestamps[mostRecentButtonPressIndex];
    absolute_time_t oldestButtonPressTs = g_globalData.buttonPressTimestamps[oldestButtonPressIndex];

    if( ( is_nil_time( mostRecentButtonPressTs ) == true )
        || ( is_nil_time( oldestButtonPressTs ) == true ) )
    {
        return false;
    }

    // Check if a repeated button press has already been registed recently
    else if( ( is_nil_time( g_globalData.repeatedButtonPressRegisteredTimestamp ) == false )
        && ( absolute_time_diff_us( g_globalData.repeatedButtonPressRegisteredTimestamp, mostRecentButtonPressTs ) < ( REPEATED_PRESSES_COOLDOWN_MS * 1000LL ) ) )
    {
        return false;
    }

    else if( absolute_time_diff_us( oldestButtonPressTs, mostRecentButtonPressTs ) < ( REPEATED_PRESSES_TIMING_MS * 1000LL ) )
    {
        g_globalData.repeatedButtonPressRegisteredTimestamp = get_absolute_time();
        return true;
    }

    return false;
}

static inline bool m_checkDoubleButtonPress( void )
{
    int16_t mostRecentButtonPressIndex = (int16_t) g_globalData.buttonPressTsIndex - 1;
    if( mostRecentButtonPressIndex < 0 )
        mostRecentButtonPressIndex += INFO_SCREEN_REPEATED_PRESSES;

    int16_t previousButtonPressIndex = (int16_t) g_globalData.buttonPressTsIndex - 2;
    if( previousButtonPressIndex < 0 )
        previousButtonPressIndex += INFO_SCREEN_REPEATED_PRESSES;

    absolute_time_t mostRecentButtonPressTs = g_globalData.buttonPressTimestamps[mostRecentButtonPressIndex];
    absolute_time_t previousButtonPressTs = g_globalData.buttonPressTimestamps[previousButtonPressIndex];

    if( ( is_nil_time( mostRecentButtonPressTs ) == true )
        || ( is_nil_time( previousButtonPressTs) == true ) )
    {
        return false;
    }

    // If a double button press has already happened recently, don't register a new one
    else if( ( is_nil_time( g_globalData.doublePressRegisteredTimestamp ) == false )
        && ( absolute_time_diff_us( g_globalData.doublePressRegisteredTimestamp, mostRecentButtonPressTs ) < ( DOUBLE_PRESS_COOLDOWN_MS * 1000LL ) ) )
    {
        return false;
    }
    else if( absolute_time_diff_us( previousButtonPressTs, mostRecentButtonPressTs ) < ( DOUBLE_PRESS_MAX_GAP_MS * 1000LL ) )
    {
        // Log the double button press
        g_globalData.doublePressRegisteredTimestamp = get_absolute_time();
        return true;
    }
    return false;
}

static inline void m_infoStateMoveToNextScreen( void )
{
    t_infoScreen newState = g_globalData.infoMode;
    bool newStateChecked = false;
    while( newStateChecked == false )
    {
        // Try a new info screen
        switch( newState )
        {
            case e_infoScreen_dryTank:
            {
                newState = e_infoScreen_binDayLandfill;
            }
            break;
            case e_infoScreen_binDayLandfill:
            {
                newState = e_infoScreen_binDayRecycling;
            }
            break;
            case e_infoScreen_binDayRecycling:
            {
                newState = e_infoScreen_standard;
            }
            break;
            case e_infoScreen_standard:
            {
                newState = e_infoScreen_dryTank;
            }
            break;
            case e_infoScreen_propaganda:
            {
                // I'm not sure what to do here yet,
                // this screen can only be dismissed by a hold
                // or by waiting
            }
            default:
            {
                newState = e_infoScreen_standard;
            }
        }
        // Check that this is a valid screen to show
        switch( newState )
        {
            case e_infoScreen_dryTank:
            {
                if( g_globalData.tankState == e_tankState_dry )
                {
                    newStateChecked = true;
                }
            }
            break;
            case e_infoScreen_binDayLandfill:
            {
                if( m_checkLandfillBinday() )
                {
                    newStateChecked = true;
                }
            }
            break;
            case e_infoScreen_binDayRecycling:
            {
                if( m_checkRecyclingBinday() )
                {
                    newStateChecked = true;
                }
            }
            break;
            case e_infoScreen_standard:
            {
                // This screen is always valid
                newStateChecked = true;
            }
            break;
            default:
            {
                printf( "default reached in m_infoStateMoveToNextScreen\n" );
            }
        }
    }
}
