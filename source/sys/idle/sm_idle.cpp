#include "sm_idle.hpp"

#include "oled.hpp"
#include "pico/cyw43_arch.h"
#include "system.hpp"

void smIdle_init( t_globalData* globalDataPtr )
{
    // Clear the screen
    oled_clear();
    oled_terminalDeinit();
}

void smIdle_update( t_globalData* globalDataPtr )
{

}
