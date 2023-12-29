#include "ntp.hpp"

#define NTP_SERVER  "pool.ntp.org"
#define NTP_PORT    ( 123 )
#define NTP_MSG_LEN ( 48 )            // Apparently this value ignores the authenticator
#define NTP_DELTA   ( 2208988800ULL ) // Seconds between 1 Jan 1900 and 1 Jan 1970

void m_dnsCallback( const char *name, const ip_addr_t *ipaddress, void *arg );
void m_ntpRecievedCallback( void *arg, struct udp_pcb *pcb, struct pbuf *p, 
    const ip_addr_t *addr, u16_t port );
int64_t alarmNtpUpdateCallback( alarm_id_t alarm_id, void* param );

typedef struct
{
    ip_addr_t ntpIpAddress;
    struct udp_pcb *ntpPcb;
    bool ntpServerFound;
    absolute_time_t ntpUpdateTime;
    int tcpipLinkState;
} t_ntpData;

t_ntpData m_ntpData;


