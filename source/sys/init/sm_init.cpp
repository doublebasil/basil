#include "sm_init.hpp"

#include "oled.hpp"
#include "pico/cyw43_arch.h"
#include "pump.hpp"
#include "sd_card.h"
#include "ff.h"
#include "settings_reader.hpp"
#include "system.hpp"

static inline void m_initialiseOled( void );
static inline void m_initialiseCyw43( void );
static inline void m_initialisePump( void );
static inline void m_initialiseSdCardDriver( void );
static inline void m_sdSuccessfulReadMessage( t_globalData* globalDataPtr );
static inline void m_sdFailedReadMessage( void );
static inline void m_attemptWifiConnection( t_globalData* globalDataPtr );

void smInit_init( t_globalData* globalDataPtr )
{
    // Initialise everything the application needs
    // Initialise parts of the global data struct that haven't been set yet
    globalDataPtr->hardwareData.displayWidth = OLED_DISPLAY_WIDTH;
    globalDataPtr->hardwareData.displayHeight = OLED_DISPLAY_HEIGHT;
    // Initialise the OLED
    m_initialiseOled();
    // Initialise the WiFi driver, also needed for built-in LED
    m_initialiseCyw43();
    // Initialise pump
    m_initialisePump();
    // Init the SD card driver
    m_initialiseSdCardDriver();
    // Now attempt to read the SD card
    oled_terminalWrite( "" );
    oled_terminalWrite( "Reading SDC..." );
    if( settings_readFromSDCard( globalDataPtr ) != 0 )
    {
        globalDataPtr->hardwareData.settingsReadOk = false;
        m_sdFailedReadMessage();
    }
    else
    {
        globalDataPtr->hardwareData.settingsReadOk = false;
        m_sdSuccessfulReadMessage( globalDataPtr );

        // Attempt to connect to wifi with the SSID and password read from the SD card
        m_attemptWifiConnection( globalDataPtr );

        // If WiFi connection was successful
    }

    // Setup a timeout for this state
    globalDataPtr->stateTimeout = make_timeout_time_ms( INIT_STATE_TIMEOUT_MS );
}

void smInit_update( t_globalData* globalDataPtr )
{
    // Check for user input
    switch( globalDataPtr->leftButtonPendingInput )
    {
        case e_pendingInput_singlePress:
        {
            // Change to the info state
            system_setState( globalDataPtr, e_systemState_info );
        }
        break;
        case e_pendingInput_spamPress:
        case e_pendingInput_none:
        default:
        {
            // Do nothing
        }
        break;
    }
    switch( globalDataPtr->rightButtonPendingInput )
    {
        case e_pendingInput_singlePress:
        {
            // Change to the info state
            system_setState( globalDataPtr, e_systemState_info );
        }
        break;
        case e_pendingInput_spamPress:
        break;
        case e_pendingInput_none:
        default:
        {
            // Do nothing
        }
        break;
    }

    // Check if the state has timed out
    if( absolute_time_diff_us( get_absolute_time(), globalDataPtr->stateTimeout ) < 0U )
    {
        // State has timed out
        system_setState( globalDataPtr, e_systemState_idle );
    }
}

// --- MODULE SCOPE FUNCTIONS ---

static inline void m_initialiseOled( void )
{
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
}

static inline void m_initialiseCyw43( void )
{
    // Intialise the CYW43 driver
    oled_terminalWrite( "CYW43 initialising" );
    if( cyw43_arch_init() )
    {
        oled_terminalWrite( "Failed" );
        for( ;; )
        {
            printf( "cyw43_arch_init failed\n" );
            sleep_ms( 1000 );
        }
    }
    // Enable STA (Station) mode
    oled_terminalWrite( "Enable STA mode" );
    cyw43_arch_enable_sta_mode();
}

static inline void m_initialisePump( void )
{
    oled_terminalWrite( "Initialising pump" );
    pump_init( PUMP_CONTROL_PIN, PUMP_ADC_PIN );
}

static inline void m_initialiseSdCardDriver( void )
{
    oled_terminalWrite( "Init SDC driver" );
    if( !sd_init_driver() )
    {
        oled_terminalWrite( "Failed" );
        for( ;; )
        {
            printf( "sd_init_driver failed\n" );
            sleep_ms( 1000 );
        }
    }
}

static inline void m_sdSuccessfulReadMessage( t_globalData* globalDataPtr )
{
    oled_terminalWrite( "Settings read" );
    oled_terminalWrite( "successfully" );
    oled_terminalWrite( "" );
    oled_terminalWrite( "Watering at:" );
    char text[20];
    int32_t secondsSinceMidnight;
    for( uint8_t index = 0U; index < MAX_NUMBER_OF_WATERING_TIMES; index++ )
    {
        secondsSinceMidnight = globalDataPtr->sdCardSettings.wateringTimes[index];
        if( secondsSinceMidnight == -1 )
            break;
        
        snprintf( text, sizeof( text ), "-> %02lld:%02lld", 
            secondsSinceMidnight / ( 60LL * 60LL ),
            ( secondsSinceMidnight % ( 60LL * 60LL ) ) / 60LL );
        
        oled_terminalWrite( text );
    }
    
    snprintf( text, sizeof( text ), "for %d ms", globalDataPtr->sdCardSettings.wateringDurationMs );
    oled_terminalWrite( text );
}

static inline void m_sdFailedReadMessage( void )
{
    oled_terminalWrite( "FAILED to read" );
    oled_terminalWrite( "settings" );
    oled_terminalWrite( "" );
    oled_terminalWrite( "Default settings:" );
    char text[20];
    snprintf( text, sizeof( text ), "Water every %d", DEFAULT_WATERING_PERIOD_HOURS );
    oled_terminalWrite( text );
    snprintf( text, sizeof( text ), "hours for %d ms", DEFAULT_WATERING_DURATION_MS );
    oled_terminalWrite( text );
}

static inline void m_attemptWifiConnection( t_globalData* globalDataPtr )
{
    oled_terminalWrite( "" );
    oled_terminalWrite( "Connecting to:" );
    oled_terminalWrite( globalDataPtr->sdCardSettings.wifiSsid );
    
    int result = cyw43_arch_wifi_connect_timeout_ms( globalDataPtr->sdCardSettings.wifiSsid, 
        globalDataPtr->sdCardSettings.wifiPassword, 
        CYW43_AUTH_WPA2_AES_PSK, 
        30000 );

    if( result == 0 )
    {
        // Connection successful
        ++( globalDataPtr->wifiData.connectionAttempts );
        globalDataPtr->wifiData.connectionSuccess = true;
        oled_terminalWrite( "Connected" );
    }
    else
    {
        // Connection failed
        ++( globalDataPtr->wifiData.connectionAttempts );
        globalDataPtr->wifiData.connectionSuccess = false;
        globalDataPtr->wifiData.reconnectionAttemptTime = make_timeout_time_ms( (uint32_t) WIFI_CONNECTION_RETRY_DELAY_MINS * 60ULL );
        
        oled_terminalWrite( "Failed" );
        oled_terminalWrite( "Will retry in" );
        char text[20];
        snprintf( text, sizeof( text ), "%d minutes", WIFI_CONNECTION_RETRY_DELAY_MINS );
        oled_terminalWrite( text );
    }
}
