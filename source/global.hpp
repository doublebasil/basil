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

typedef enum
{
    e_infoScreen_dryTank,
    e_infoScreen_binDayRecycling,
    e_infoScreen_binDayLandfill,
    e_infoScreen_standardInfo,
} t_infoScreenCurrentInfo;

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
    bool stateHasInitialised; // Ensure you use m_setSystemState so that this variable gets set to false
    t_tankState tankState;
    /* STATE SPECIFIC VARIABLES */
    bool notificationDisplayed; // For idle state
    t_infoScreenCurrentInfo currentInfo; // For info state
    /* TIME STAMPS */
    absolute_time_t stateStartTimestamp;
    absolute_time_t screenTimeoutTs;
    absolute_time_t nextWateringTs;
    absolute_time_t nextRecyclingBinday; // Bin day time stamps are the midnight before
    absolute_time_t nextLandfillBinday;
    /* WEBSERVER SETTINGS */
    bool webServerRunning;
} t_globalData;

#endif // defined GLOBAL_HPP
