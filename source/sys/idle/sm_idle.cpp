#include "sm_idle.hpp"

#include "oled.hpp"
#include "pico/cyw43_arch.h"
#include "system.hpp"

static bool m_checkWifiReconnection( t_globalData* globalDataPtr );

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
    // Check if a reconnection to WiFi is needed
    if( m_checkWifiReconnection( globalDataPtr ) )
        system_setState( globalDataPtr, e_systemState_wifi );
    // else if 
}

static bool m_checkWifiReconnection( t_globalData* globalDataPtr )
{
    if( is_nil_time( globalDataPtr->wifiData.reconnectionAttemptTime ) )
    {
        return false; // Connection reattempt not needed
    }
    else if( absolute_time_diff_us( get_absolute_time(), globalDataPtr->wifiData.reconnectionAttemptTime ) < 0LL )
    {
        return true; // Time to reattempt a connection 
    }
    else
    {
        return false;
    }
}
