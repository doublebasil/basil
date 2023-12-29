#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"

// /* MODE 1 - RTC/NTP, and motor driver
//  * MODE 2 - RTC/NTP, web server, SD card, OLED (Not being implemented yet)
//  * MODE 3 - Everything from mode 2, but also UV level logging */
// #define MODE    ( 1 )

#include "wifi_settings.h"

// #include "sd_card.h"
// #include "ff.h"
// #include "oled.hpp"

// #define TERMINAL_COLOUR  ( 0xFD44 )

static inline void m_rebootBoard();
static inline void m_cyw43Init();
// static inline void m_oledInit();
// static inline void m_sdCardInit();

// CHANGE THE POWER SAVING MODE WHEN LAUNCHING THE WEB SERVER

int main( void )
{
    char textBuffer[19];

    // Initialise the debug output
    stdio_init_all();
    // printf statements may not work for about a second after calling the stdio init

    // WiFi driver initialisation
    m_cyw43Init();

    // // OLED screen initialisation
    // m_oledInit();

    // // Create a terminal for the display  "MAX TERMINAL WIDTH"
    // oled_terminalInit( 12, TERMINAL_COLOUR );
    // oled_terminalWrite( "OLED initialised" );
    // oled_terminalWrite( "CYW43 initialised" );

    // oled_terminalWrite( " _           _  __ " );
    // oled_terminalWrite( "|_) _. _ o| / \\(_  " );
    // oled_terminalWrite( "|_)(_|_> || \\_/__) " );
    // oled_terminalWrite( " " );

    // Flicker the built in LED to indicate the pico has booted
    for( int i = 0; i < 3; i++ )
    {
        cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 1 );
        sleep_ms( 100 );
        cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 0 );
        sleep_ms( 100 );
    }

    // Enable station mode for the WiFi driver
    cyw43_arch_enable_sta_mode();
    // oled_terminalWrite( "Enabled STA mode" );

    // // SD card driver init
    // oled_terminalWrite( "Initialising SDC" );
    // m_sdCardInit();
    // oled_terminalWrite( "Done!" );

    // GPIO, ADC, etc
    // gpio_init();

    // Connect to WiFi
    // oled_terminalWrite( "Connecting to SSID" );
    // snprintf( textBuffer, sizeof(textBuffer), "\"%s\"", WIFI_SSID );
    // oled_terminalWrite( textBuffer );

    cyw43_arch_wifi_connect_timeout_ms( WIFI_SSID,
        WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000 );

    sleep_ms( 1000 );
    
    // m_rebootBoard();

    printf( "Running\n" );

    for( ;; ) {}
}

static inline void m_rebootBoard()
{
    // Reboot the board by enabling a 1ms watchdog, and then not feeding it
    watchdog_enable( 1, false );
    for( ;; ) {}
}

static inline void m_cyw43Init()
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

// static inline void m_oledInit()
// {
//     if( oled_init( 19, 18, 17, 16, 20, 0, 14000000, 128, 128 ) != 0 )
//     {
//         cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 1 );
//         for( ;; )
//         {
//             printf( "oled_init failed\n" );
//             sleep_ms( 1000 );
//         }
//     }
// }

// static inline void m_sdCardInit()
// {
//     if( !sd_init_driver() )
//     {
//         cyw43_arch_gpio_put( CYW43_WL_GPIO_LED_PIN, 1 );
//         for( ;; )
//         {
//             printf( "sd_init_driver failed\n" );
//             sleep_ms( 1000 );
//         }
//     }
// }
