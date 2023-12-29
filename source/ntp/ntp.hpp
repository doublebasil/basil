#ifndef NTP_HPP
#define NTP_HPP

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#define NTP_SERVER  "pool.ntp.org"
#define NTP_PORT    ( 123 )
#define NTP_MSG_LEN ( 48 )      // Apparently this value ignores the authenticator
#define NTP_DELTA (2208988800)  // Seconds between 1 Jan 1900 and 1 Jan 1970

#endif // ifndef NTP_HPP
