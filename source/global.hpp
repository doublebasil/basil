#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "pico/stdlib.h"
#include "pico/time.h"

#define WIFI_SSID_MAX_LEN               ( 32 )
#define WIFI_PASSWORD_MAX_LEN           ( 32 )
#define MAX_NUMBER_OF_WATERING_TIMES    ( 5 )

#define MAX_NUMBER_OF_TASKS             ( 5 )

typedef struct 
{
    bool taskActive = false;
    absolute_time_t taskScheduleTime;
    void (*callback)( void );
} t_task;

typedef enum
{
    e_displayState_clear,
    e_displayState_terminal,
    e_displayState_image,
} t_displayState;

typedef struct
{
    /* SD CARD SETTINGS */
    bool settingsReadOk;
    char wifiSsid[WIFI_SSID_MAX_LEN];
    char wifiPassword[WIFI_PASSWORD_MAX_LEN];
    int32_t wateringTimes[MAX_NUMBER_OF_WATERING_TIMES]; // Seconds since midnight, only used if RTC is set via NTP server
    uint16_t wateringDurationMs;
    /* ALARMS */
    alarm_id_t alarmIdWatering;
    /* DISPLAY STATUS */
    t_displayState displayState;

    // Pump current status
    // bool pumpCurrentlyOn;

    // Webserver current status
    // bool webServerRunning;
} t_globalData;

#endif // defined GLOBAL_HPP
