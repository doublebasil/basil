#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <stdio.h>
#include "pico/stdlib.h"

/* --- HARDWARE DEFINITIONS --- */
/* OLED */
#define OLED_DATA_IN_PIN        ( 19 )
#define OLED_CLOCK_PIN          ( 18 )
#define OLED_CHIP_SELECT_PIN    ( 17 )
#define OLED_DATA_COMMAND_PIN   ( 16 )
#define OLED_RESET_PIN          ( 20 )
#define OLED_SPI_OUTPUT         ( 0 )
#define OLED_BAUD_RATE_HZ       ( 14000000 ) // 14 MHz
#define OLED_DISPLAY_WIDTH      ( 128 )
#define OLED_DISPLAY_HEIGHT     ( 128 )
/* PUMP */
#define PUMP_CONTROL_PIN        ( 21 )
#define PUMP_ADC_PIN            ( 26 )
/* INPUT BUTTONS */
#define LEFT_BUTTON_PIN         ( 3 )
#define RIGHT_BUTTON_PIN        ( 2 )
/* SD CARD SETTINGS */
// Settings for the SD card are defined in source/no-OS-FatFS-SD-SPI-Rpi-Pico/FatFs_SPI/sd_driver/hw_config.c

/* --- OTHER DEFINITIONS --- */
#define WIFI_SSID_MAX_LEN               ( 32 )
#define WIFI_PASSWORD_MAX_LEN           ( 32 )
#define MAX_NUMBER_OF_WATERING_TIMES    ( 5 )

/* --- RGB565 COLOURS --- */
#define RGB565_FOREST_GREEN             ( 0x2444 )

/* --- TERMINAL --- */
#define TERMINAL_FONT_12                ( 12 )
#define TERMINAL_FONT_20                ( 20 )
#define TERMINAL_INIT_COLOUR            ( RGB565_FOREST_GREEN )

/* --- TIMING AND BEHAVIOURS --- */
#define MAIN_LOOP_TIME_PERIOD_MS        ( 50LL )
#define SPAM_PRESS_COUNT                ( 10 ) // Number of quick presses to trigger a "spam press"
#define SPAM_PRESS_TIME_LIMIT_MS        ( 1500LL )
#define INIT_STATE_TIMEOUT_MS           ( 10000LL )

/* --- TYPEDEFS --- */
typedef enum {
    e_systemState_notSet,
    e_systemState_init,
    e_systemState_idle,
    e_systemState_info,
    e_systemState_watering,
} t_systemState;

typedef enum {
    e_tankState_unknown,
    e_tankState_ok,
    e_tankState_dry,
} t_tankState;

typedef enum { // IN REVERSE ORDER OF IMPORANCE
    e_pendingInput_none,
    e_pendingInput_singlePress,
    e_pendingInput_spamPress,
} t_pendingInput;

/* SUB STRUCTS */
typedef struct {
    char wifiSsid[WIFI_SSID_MAX_LEN];
    char wifiPassword[WIFI_PASSWORD_MAX_LEN];
    int32_t wateringTimes[MAX_NUMBER_OF_WATERING_TIMES]; // Seconds since midnight, only used if RTC is set via NTP server
    uint16_t wateringDurationMs;
} t_sdCardSettings;

typedef struct {
    bool settingsReadOk;
    uint8_t displayWidth;
    uint8_t displayHeight;
} t_hardwareData;

/* GLOBAL DATA STRUCT */
typedef struct {
    /* SUB STRUCTS */
    t_sdCardSettings sdCardSettings;
    t_hardwareData hardwareData;
    /* STATE MODEL */
    t_systemState systemState = e_systemState_notSet;
    t_tankState tankState = e_tankState_unknown;
    absolute_time_t stateTimeout = nil_time;
    /* INPUTS */
    t_pendingInput leftButtonPendingInput;
    t_pendingInput rightButtonPendingInput;
} t_globalData;

#endif // SETTINGS_HPP
