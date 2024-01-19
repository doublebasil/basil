#include "sm_wifi.hpp"

#include "oled.hpp"
#include "pico/cyw43_arch.h"
#include "system.hpp"

static void m_connectToWifi( t_globalData* globalDataPtr )

void smWifi_init( t_globalData* globalDataPtr )
{
    // Clear the screen
    oled_clear();
    oled_deinitAll();

    // Setup a terminal
    oled_terminalInit( 12, RGB565_PURE_BLUE );

    // Attempt to wifi if needed
    m_connectToWifi();

    // Create a timeout for this state
    globalDataPtr->stateTimeout = make_timeout_time_ms( WIFI_STATE_TIMEOUT_MS );
}

void smWifi_update( t_globalData* globalDataPtr )
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

static void m_connectToWifi( t_globalData* globalDataPtr )
{
    // Check if a connection to wifi is needed
    if( globalDataPtr->wifiData.connectionSuccess == true )
        return; // Connection already present
    else if( globalDataPtr->wifiData.connectionAttempts == WIFI_CONNECTION_MAX_ATTEMPTS)
    {
        // Too many connection attempts
        globalDataPtr->wifiData.reconnectionAttemptTime = nil_time;

        oled_terminalWrite( "Max connection" );
        oled_terminalWrite( "attempts reached" );
    }
    else
    {
        // Attempt to connect
        // YOU WERE HERE, SEE BELOW COMMENTED FUNCTION
    }
}

// static inline void m_attemptWifiConnection( t_globalData* globalDataPtr )
// {
//     oled_terminalWrite( "" );
//     oled_terminalWrite( "Connecting to:" );
//     oled_terminalWrite( globalDataPtr->sdCardSettings.wifiSsid );
    
//     int result = cyw43_arch_wifi_connect_timeout_ms( globalDataPtr->sdCardSettings.wifiSsid, 
//         globalDataPtr->sdCardSettings.wifiPassword, 
//         CYW43_AUTH_WPA2_AES_PSK, 
//         30000 );

//     if( result == 0 )
//     {
//         // Connection successful
//         ++( globalDataPtr->wifiData.connectionAttempts );
//         globalDataPtr->wifiData.connectionSuccess = true;
//         oled_terminalWrite( "Connected" );
//     }
//     else
//     {
//         // Connection failed
//         ++( globalDataPtr->wifiData.connectionAttempts );
//         globalDataPtr->wifiData.connectionSuccess = false;
//         globalDataPtr->wifiData.reconnectionAttemptTime = make_timeout_time_ms( (uint32_t) WIFI_CONNECTION_RETRY_DELAY_MINS * 60ULL );
        
//         oled_terminalWrite( "Failed" );
//         oled_terminalWrite( "Will retry in" );
//         char text[20];
//         snprintf( text, sizeof( text ), "%d minutes", WIFI_CONNECTION_RETRY_DELAY_MINS );
//         oled_terminalWrite( text );
//     }
// }