#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "pico/stdlib.h"
#include "pico/time.h"

#define WIFI_SSID_MAX_LEN               ( 32 )
#define WIFI_PASSWORD_MAX_LEN           ( 32 )
#define MAX_NUMBER_OF_WATERING_TIMES    ( 5 )
// Number of quick button presses on info screen to 
#define INFO_SCREEN_REPEATED_PRESSES    ( 10 )
#define REPEATED_PRESSES_TIMING_MS      ( 2000LL )
#define REPEATED_PRESSES_COOLDOWN_MS    ( 15000LL )

#define LONG_PRESS_MIN_TIME_MS          ( 300LL ) // How long until the long press screen is displayed
#define LONG_PRESS_ACTIVATE_TIME_MS     ( 2000LL ) // How long until the long press function activates
#define DOUBLE_PRESS_MAX_GAP_MS         ( 300LL )
#define DOUBLE_PRESS_COOLDOWN_MS        ( 300LL )

#define HELP_SCREEN_TIMEOUT_MS          ( 5000LL )

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
    e_infoScreen_none,
    e_infoScreen_dryTank,
    e_infoScreen_binDayRecycling,
    e_infoScreen_binDayLandfill,
    e_infoScreen_standard,
    e_infoScreen_propaganda,
} t_infoScreen;

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
    /* IDLE STATE SPECIFIC DATA */
    bool notificationDisplayed;
    /* INFO STATE SPECIFIC DATA */
    t_infoScreen infoMode;
    t_infoScreen displayedInfo;
    // absolute_time_t currentInfoTimeoutTimestamp; // Think I won't do this
    absolute_time_t buttonPressTimestamps[INFO_SCREEN_REPEATED_PRESSES];
    uint8_t buttonPressTsIndex;
    absolute_time_t doublePressRegisteredTimestamp;
    absolute_time_t repeatedButtonPressRegisteredTimestamp;
    bool showHelpScreen; // Access through a single button press
    bool helpScreenInitialised;
    bool showLongPressScreen;
    absolute_time_t altScreenTimeoutTimestamp; // Timeout for the help screen or the long press screen
    /* TIME STAMPS */
    // absolute_time_t stateStartTimestamp;
    absolute_time_t screenTimeoutTs;
    absolute_time_t nextWateringTs;
    absolute_time_t nextRecyclingBinday; // Bin day timestamps are the midnight before
    absolute_time_t nextLandfillBinday;
    /* WEBSERVER SETTINGS */
    bool webServerRunning;
} t_globalData;

#endif // defined GLOBAL_HPP
