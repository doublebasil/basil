#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "pico/stdlib.h"
#include "pico/time.h"

#define WIFI_SSID_MAX_LEN               ( 32 )
#define WIFI_PASSWORD_MAX_LEN           ( 32 )
#define MAX_NUMBER_OF_WATERING_TIMES    ( 5 )

#define MAX_NUMBER_OF_TASKS             ( 5 )

/* ENUMERATIONS */

// typedef enum
// {
//     e_displayState_clear,
//     e_displayState_initTerminal,
//     e_displayState_infoTerminal,
//     e_displayState_image,
// } t_displayState;

typedef enum
{
    e_systemState_init, // The initial terminal screen will be shown
    e_systemState_idle, // Nothing is shown on the screen
    e_systemState_info, // Information is shown on the screen
    e_systemState_startWatering, // The loading donut is shown on the screen
    e_systemState_postWatering, // Post watering clarity
} t_systemState;

typedef enum
{
    e_tankState_unknown,
    e_tankState_ok,
    e_tankState_dry,
} t_tankState;

typedef enum
{
    e_binday_none,
    e_binday_recycling,
    e_binday_landfill,
} t_binday;

/* STRUCT */

typedef struct
{
    /* SD CARD SETTINGS */
    bool settingsReadOk;
    char wifiSsid[WIFI_SSID_MAX_LEN];
    char wifiPassword[WIFI_PASSWORD_MAX_LEN];
    int32_t wateringTimes[MAX_NUMBER_OF_WATERING_TIMES]; // Seconds since midnight, only used if RTC is set via NTP server
    uint16_t wateringDurationMs;
    /* STATES */
    t_binday bindayState;
    // t_displayState displayState;
    t_tankState tankState;
    t_systemState systemState;
    /* TIME STAMPS */
    absolute_time_t screenTimeoutTs;
    absolute_time_t nextWateringTs;

    // Webserver current status
    // bool webServerRunning;
} t_globalData;

#endif // defined GLOBAL_HPP
