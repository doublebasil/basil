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
static inline void m_sdSuccessfulReadMessage( void );
static inline void m_sdFailedReadMessage( void );

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
    oled_terminalWrite( "Reading SDC...");
    if( settings_readFromSDCard( globalDataPtr ) != 0 )
    {
        globalDataPtr->hardwareData.settingsReadOk = false;
        m_sdFailedReadMessage();
    }
    else
    {
        globalDataPtr->hardwareData.settingsReadOk = false;
        m_sdSuccessfulReadMessage();
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
            printf( "left button single press\n" );
        }
        break;
        case e_pendingInput_spamPress:
        {
            printf( "left button REPEATED PRESS\n" );
        }
        break;
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
            printf( "right button single press\n" );
        }
        break;
        case e_pendingInput_spamPress:
        {
            printf( "right button REPEATED PRESS\n" );
        }
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

static inline void m_sdSuccessfulReadMessage( void )
{
    oled_terminalWrite( "Settings read" );
    oled_terminalWrite( "successfully" );
    oled_terminalWrite( "" );
    oled_terminalWrite( "Watering at:" );
    char text[20];
    for( uint8_t index = 0U; index < MAX_NUMBER_OF_WATERING_TIMES; index++ )
    {
        snprintf( text, sizeof( text ), "%02d:%02d:%02d", 
             );
    }

    /*
    
      uint64_t t = 65700;
  uint16_t h = (t / (60ULL * 60ULL));
//   uint16_t m = (uint8_t) (( t - ((uint64_t) h * 60ULL * 60ULL) ) / (60ULL));
  uint16_t m = (t % (60ULL * 60ULL)) / (60ULL);
  cout << h << endl;
  cout << m << endl;
    
    */
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
