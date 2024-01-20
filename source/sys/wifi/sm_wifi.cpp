#include "sm_wifi.hpp"

#include "oled.hpp"
#include "pico/cyw43_arch.h"
#include "system.hpp"

/* MODULE SCOPE FUNCTION PROTOTYPE */
static void m_connectToWifi( t_globalData* globalDataPtr );
/* RTC/NTP RELATED */


/* PUBLIC FUNCTION IMPLEMENTATIONS */
void smWifi_init( t_globalData* globalDataPtr )
{
    // Clear the screen 
    oled_clear();
    oled_deinitAll();
    // Make a new terminal
    oled_terminalInit( 12, TERMINAL_WIFI_COLOUR );
    oled_terminalWrite( " < < WiFi > >" );

    // Attempt to wifi if needed
    m_connectToWifi( globalDataPtr );
    // If wifi connection is present, attempt to connect to an NTP server

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

/* MODULE SCOPE FUNCTION PROTOTYPE */
static void m_connectToWifi( t_globalData* globalDataPtr )
{
    char text[20];

    // Check if a connection to wifi is needed
    if( globalDataPtr->wifiData.connectionSuccess == true )
    {
        return; // Connection already present
    }
    else if( globalDataPtr->wifiData.connectionAttempts == WIFI_CONNECTION_MAX_ATTEMPTS )
    {
        // Too many connection attempts
        globalDataPtr->wifiData.reconnectionAttemptTime = nil_time;

        oled_terminalWrite( "Max connection" );
        oled_terminalWrite( "attempts reached" );
    }
    else
    {
        // Attempt to connect
        oled_terminalWrite( "Connecting to:" );
        oled_terminalWrite( globalDataPtr->sdCardSettings.wifiSsid );

        int result = cyw43_arch_wifi_connect_timeout_ms( globalDataPtr->sdCardSettings.wifiSsid, 
            globalDataPtr->sdCardSettings.wifiPassword, 
            CYW43_AUTH_WPA2_AES_PSK, 
            30000 );

        ++( globalDataPtr->wifiData.connectionAttempts );

        if( result == 0 )
        {
            globalDataPtr->wifiData.connectionSuccess = true;
            globalDataPtr->wifiData.reconnectionAttemptTime = nil_time;

            oled_terminalWrite( "" );
            oled_terminalWrite( "Success" );
        }
        else
        {
            globalDataPtr->wifiData.connectionSuccess = false;

            oled_terminalWrite( "" );
            oled_terminalWrite( "Failed" );

            if( globalDataPtr->wifiData.connectionAttempts == WIFI_CONNECTION_MAX_ATTEMPTS )
            {
                globalDataPtr->wifiData.reconnectionAttemptTime = nil_time;

                oled_terminalWrite( "" );
                oled_terminalWrite( "Max connection" );
                oled_terminalWrite( "attempts reached" );
            }
            else
            {
                globalDataPtr->wifiData.reconnectionAttemptTime = make_timeout_time_ms( (uint32_t) WIFI_CONNECTION_RETRY_DELAY_MINS * 60LL * 1000LL );

                oled_terminalWrite( "Retry scheduled" );
                if( WIFI_CONNECTION_RETRY_DELAY_MINS == 1 )
                    snprintf( text, sizeof( text ), "in %d minute", WIFI_CONNECTION_RETRY_DELAY_MINS );
                else
                    snprintf( text, sizeof( text ), "in %d minutes", WIFI_CONNECTION_RETRY_DELAY_MINS );
                
                oled_terminalWrite( text );
            }
        }
    }
}

