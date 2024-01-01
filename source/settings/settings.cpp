#include "settings.hpp"

typedef enum
{
    e_sdSettingRead_wifiSsid,
    e_sdSettingRead_wifiPassword,
    e_sdSettingRead_wateringTimes,
    e_sdSettingRead_wateringDuration,
    e_sdSettingRead_done,
    e_sdSettingRead_bufferOverfull,
} t_sdCardReadCurrentSetting;

int settings_readFromSDCard( t_globalData* globalDataPtr )
{
    
}

// static int settings_writeToSDCard(); // TODO
