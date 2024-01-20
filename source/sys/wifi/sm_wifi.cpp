#include "sm_wifi.hpp"

#include "oled.hpp"
#include "system.hpp"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

/* RTC/NTP RELATED DEFINITIONS */
typedef struct {
    ip_addr_t ntpServerAddress;
    bool dnsRequestSent;
    struct udp_pcb *ntpPcb; // User Datagram Protocol, Network Time Protocol, Protocol Control Block
    absolute_time_t ntpTestTime;
} t_ntp;
#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

static t_ntp m_ntpState;
static bool m_ntpStateInitialised = false;

/* MODULE SCOPE FUNCTION PROTOTYPE */
static void m_connectToWifi( t_globalData* globalDataPtr );
/* RTC/NTP RELATED FUNCTIONS */
static void m_setRtcFromNtp( t_globalData* globalDataPtr );
static void m_ntpReceivedCb( void *arg, struct udp_pcb *pcb, struct pbuf *p, 
    const ip_addr_t *addr, u16_t port );
static void m_ntpResult( t_ntp* ntpStatePtr, int status, time_t *result );

/* PUBLIC FUNCTION IMPLEMENTATIONS */
void smWifi_init( t_globalData* globalDataPtr )
{
    // Clear the screen 
    oled_clear();
    oled_deinitAll();
    // Make a new terminal
    oled_terminalInit( 12, TERMINAL_WIFI_COLOUR );
    oled_terminalSetHeight( 5 );

    // Attempt to wifi if needed
    m_connectToWifi( globalDataPtr );
    // If wifi connection is present, attempt to connect to an NTP server
    m_setRtcFromNtp( globalDataPtr );

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

        oled_sdWriteImage( "wifi64.txt", 0, 64 );
        oled_sdWriteImage( "cross64.txt", 64, 64 );
    }
    else
    {
        // Attempt to connect
        oled_sdWriteImage( "wifi64.txt", 0, 64 );
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
            oled_sdWriteImage( "tick64.txt", 64, 64 );
        }
        else
        {
            globalDataPtr->wifiData.connectionSuccess = false;

            oled_terminalWrite( "" );
            oled_terminalWrite( "Failed" );
            oled_sdWriteImage( "cross49.txt", 69, 44 );

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
    oled_terminalWrite( "" );
}

/* RTC/NTP RELATED */
static void m_setRtcFromNtp( t_globalData* globalDataPtr )
{
    if( globalDataPtr->wifiData.connectionSuccess == false )
        return; // No WiFi connection
    
    if( globalDataPtr->wifiData.ntpFatal == true )
        return; // Something unfixable has happened

    if( m_ntpStateInitialised == false )
    {
        oled_terminalWrite( "Init NTP data" );
        m_ntpState.ntpPcb = udp_new_ip_type(IPADDR_TYPE_ANY);
        if( !m_ntpState.ntpPcb )
        {
            // PCB could not be allocated
            printf( "NTP failed to allocate PCB\n" );
            globalDataPtr->wifiData.ntpFatal = true; // This is so sad
        }
        // Setup a callback function for when NTP is fetched
        udp_recv( m_ntpState.ntpPcb, m_ntpReceivedCb, &m_ntpState );
    }
}

static void m_ntpReceivedCb( void *arg, struct udp_pcb *pcb, struct pbuf *p, 
    const ip_addr_t *addr, u16_t port )
{
    t_ntp* ntpStatePtr = (t_ntp*) arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if( ( ip_addr_cmp( addr, &(ntpStatePtr->ntpServerAddress) ) )
        && ( port == NTP_PORT ) 
        && ( p->tot_len == NTP_MSG_LEN )
        && ( mode == 0x4 )
        && ( stratum != 0 ) )
    {
        uint8_t secondsBuf[4] = {0};
        pbuf_copy_partial(p, secondsBuf, sizeof(secondsBuf), 40);
        uint32_t secondsSince1900 = secondsBuf[0] << 24 | secondsBuf[1] << 16 | secondsBuf[2] << 8 | secondsBuf[3];
        uint32_t secondsSince1970 = secondsSince1900 - NTP_DELTA;
        time_t epoch = secondsSince1970;
        m_ntpResult( ntpStatePtr, 0, &epoch );
    }
    else
    {
        printf( "Received an invalid NTP response\n" );
        m_ntpResult( ntpStatePtr, -1, NULL );
    }
    pbuf_free(p);
}

static void m_ntpResult( t_ntp* ntpStatePtr, int status, time_t *result )
{
    // YOU WERE HERE
}