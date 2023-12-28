#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/cyw43_arch.h"

#include "sd_card.h"
#include "ff.h"

#include "oled.hpp"

int main( void )
{
    // Initialise the debug output
    stdio_init_all();
    // printf statements may not work for about a second after calling the stdio init

    // WiFi driver initialisation
    if( cyw43_arch_init() )
    {
        for( ;; )
        {
            printf( "cyw43_arch_init failed\n" );
            sleep_ms( 1000 );
        }
    }
    // OLED screen initialisation
    if( oled_init( 19, 18, 17, 16, 20, 0, 14000000, 128, 128 ) != 0 )
    {
        for( ;; )
        {
            printf( "oled_init failed\n" );
            sleep_ms( 1000 );
        }
    }
    // SD card driver init


    printf("Running\n");

    for( ;; ) {}
}
