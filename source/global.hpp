#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "pico/stdlib.h"

#define WIFI_SSID_MAX_LEN               ( 32 )
#define WIFI_PASSWORD_MAX_LEN           ( 32 )
#define MAX_NUMBER_OF_WATERING_TIMES    ( 5 )

typedef struct
{
    // SD card settings
    bool settingsReadOk;
    char wifiSsid[WIFI_SSID_MAX_LEN];
    char wifiPassword[WIFI_PASSWORD_MAX_LEN];
    int16_t wateringTimes[MAX_NUMBER_OF_WATERING_TIMES];
    uint16_t wateringDurationMs;
} t_globalData;

#endif // defined GLOBAL_HPP
