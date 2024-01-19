#include "sm_idle.hpp"

#include "oled.hpp"
#include "pico/cyw43_arch.h"
#include "system.hpp"

void smIdle_init( t_globalData* globalDataPtr )
{
    // Clear the screen
    oled_clear();
    oled_deinitAll();
}

void smIdle_update( t_globalData* globalDataPtr )
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

    // This state cannot time out

    // Check if something needs doing
    
}
