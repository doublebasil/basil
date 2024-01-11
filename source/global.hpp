#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "pico/stdlib.h"
#include "pico/time.h"

#define WIFI_SSID_MAX_LEN               ( 32 )
#define WIFI_PASSWORD_MAX_LEN           ( 32 )
#define MAX_NUMBER_OF_WATERING_TIMES    ( 5 )

#define MAX_NUMBER_OF_TASKS             ( 5 )

/* ENUMERATIONS */

typedef enum
{
    e_systemState_init,
    e_systemState_idle,
    e_systemState_info,
    e_systemState_watering,
} t_systemState;

typedef enum
{
    e_tankState_unknown,
    e_tankState_ok,
    e_tankState_dry,
} t_tankState;

/* GLOBAL DATA STRUCT */

typedef struct
{
    /* WIFI SETTINGS */
    char wifiSsid[WIFI_SSID_MAX_LEN];
    char wifiPassword[WIFI_PASSWORD_MAX_LEN];
    /* WATERING SETTINGS */
    int32_t wateringTimes[MAX_NUMBER_OF_WATERING_TIMES]; // Seconds since midnight, only used if RTC is set via NTP server
    uint16_t wateringDurationMs;
    /* HARDWARE INFO */
    bool settingsReadOk;
    uint8_t displayWidth;
    uint8_t displayHeight;
    uint8_t buttonPin;
    /* STATES */
    t_systemState systemState;
    t_systemState previousSystemState;
    t_tankState tankState;
    /* TIME STAMPS */
    absolute_time_t screenTimeoutTs;
    absolute_time_t nextWateringTs;
    absolute_time_t nextRecyclingBinday;
    absolute_time_t nextLandfillBinday;
    /* WEBSERVER SETTINGS */
    bool webServerRunning;
} t_globalData;

#endif // defined GLOBAL_HPP
