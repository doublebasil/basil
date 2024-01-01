#include "settings.hpp"

#include "stdio.h"

#define SD_CARD_READ_BUFFER_SIZE    ( 100 )
#define CURRENT_SETTING_BUFFER_SIZE ( 50 )

typedef enum
{
    e_wifiSsid,
    e_wifiPassword,
    e_wateringTimes,
    e_wateringDuration,
    e_done,
    e_bufferOverfull,
    e_settingsReadError,
} t_sdCardReadCurrentSetting;

int m_readSetting( t_globalData* globalDataPtr, t_sdCardReadCurrentSetting currentSetting, const char settingsBuffer[] );

int settings_readFromSDCard( t_globalData* globalDataPtr )
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    char buf[SD_CARD_READ_BUFFER_SIZE];
    char filename[] = "settings.txt";

    // Create a buffer which will store the text within quote marks
    char currentSettingBuffer[CURRENT_SETTING_BUFFER_SIZE];
    uint8_t currentSettingBufferIndex = 0U;
    // Create a variable to remember which setting we are trying to read
    t_sdCardReadCurrentSetting currentSetting = e_wifiSsid;

    // Mount the SD card
    fr = f_mount( &fs, "0:", 1 );
    if( fr != FR_OK )
        return 1;

    // Open the settings file
    fr = f_open( &fil, filename, FA_READ );
    if( fr != FR_OK )
        return 2;
    
    /* This function should read the following:
     *      WIFI SSID
     *      WIFI Password
     *      Watering times
     *      Watering duration
     */
    bool withinQuotes = false; // Is the buffer index currently within quotes
    while( f_gets( buf, sizeof( buf ), &fil ) )
    {
        for( uint8_t bufferIndex = 0U; bufferIndex < sizeof( buf ); bufferIndex++ )
        {
            if( buf[bufferIndex] == 0 )
                break; // End of buffer
            else if( buf[bufferIndex] == 10 )
                continue; // Newline in file
            else if( withinQuotes == true )
            {
                // The index is currently within quotes
                // If we read another quotemark we have reached the end of quotes
                if( buf[bufferIndex] == '"' )
                {
                    withinQuotes = false;
                    currentSettingBuffer[currentSettingBufferIndex] = 0; // Show end of string
                    
                    // Process the current setting buffer
                    if( m_readSetting( globalDataPtr, currentSetting, currentSettingBuffer ) != 0 )
                    {
                        currentSetting = e_settingsReadError;
                        break;
                    }

                    // Adjust variables ready for next setting
                    currentSettingBufferIndex = 0U;
                    if( currentSetting == e_wifiSsid )
                        currentSetting = e_wifiPassword;
                    else if( currentSetting == e_wifiPassword )
                        currentSetting = e_wateringTimes;
                    else if( currentSetting == e_wateringTimes )
                        currentSetting = e_wateringDuration;
                    else if( currentSetting == e_wateringDuration )
                    {
                        currentSetting = e_done;
                        break;
                    }
                }
                // If we read something that isn't a quotemark, add it to the currentSettingBuffer
                else
                {
                    currentSettingBuffer[currentSettingBufferIndex] = buf[bufferIndex];
                    ++currentSettingBufferIndex;

                    // Check that the buffer isn't overfull,
                    // bear in mind we need space for a 0 at the end of the buffer
                    if( currentSettingBufferIndex >= ( CURRENT_SETTING_BUFFER_SIZE - 1U ) )
                    {
                        currentSetting = e_bufferOverfull;
                        break;
                    }
                }
            }
            else // withinQuotes == false
            {
                // All we're looking for is quotemarks
                if( buf[bufferIndex] == '"' )
                    withinQuotes = true;
            }
        }
        if( ( currentSetting == e_done ) ||
            ( currentSetting == e_bufferOverfull ) ||
            ( currentSetting == e_settingsReadError ) )
        {
            break;
        }
    }

    // Close the file
    fr = f_close( &fil );
    if( fr != FR_OK )
    {
        f_unmount( "0:" );
        return 3;
    }

    // Unmount the SD card
    f_unmount( "0:" );

    // Error code if buffer got too full
    if( currentSetting == e_bufferOverfull )
        return 4;
    // Error code if reading a specific setting went wrong
    if( currentSetting == e_settingsReadError )
        return 5;
    // Error code if file ended but we didn't get all the settings
    if( currentSetting != e_done )
        return 6;

    // Otherwise exit successfully
    return 0;
}

// static int settings_writeToSDCard(); // TODO

int m_readSetting( t_globalData* globalDataPtr, t_sdCardReadCurrentSetting currentSetting, const char settingsBuffer[] )
{
    if( currentSetting == e_wifiSsid )
    {
        bool stringWasTerminated = false;
        // Copy the settingBuffer string to the global data struct
        // Also ensure the string in the buffer is terminated with a 0
        for( uint8_t index = 0U; index < CURRENT_SETTING_BUFFER_SIZE; index++ )
        {
            if( index >= WIFI_SSID_MAX_LEN )
            {
                // Outside of wifiSsid string bounds, error
                return 1;
            }
            globalDataPtr->wifiSsid[index] = settingsBuffer[index];
            if( settingsBuffer[index] == 0 )
            {
                stringWasTerminated = true;
                break;
            }
        }
        if( stringWasTerminated == false )
        {
            // Bad news, never reached the end of the string
            return 1;
        }
    }
    else if( currentSetting == e_wifiPassword )
    {
        bool stringWasTerminated = false;
        // Copy the settingBuffer string to the global data struct
        // Also ensure the string in the buffer is terminated with a 0
        for( uint8_t index = 0U; index < CURRENT_SETTING_BUFFER_SIZE; index++ )
        {
            if( index >= WIFI_PASSWORD_MAX_LEN )
            {
                // Outside of wifiPassword string bounds, error
                return 1;
            }
            globalDataPtr->wifiPassword[index] = settingsBuffer[index];
            if( settingsBuffer[index] == 0 )
            {
                stringWasTerminated = true;
                break;
            }
        }
        if( stringWasTerminated == false )
        {
            // Never reached the end of the string
            return 1;
        }
    }
    else if( currentSetting == e_wateringTimes )
    {
        uint8_t currentTimeIndex = 0U;
        // Should be in 4 digit military time, comma delimited
        // Convert 24 hour time into minutes since midnight 
        
        // Find the end of the string
        uint8_t bufferIndex;
        for( bufferIndex = 0; bufferIndex < CURRENT_SETTING_BUFFER_SIZE; bufferIndex++ )
        {
            if( settingsBuffer[bufferIndex] == 0 )
                break;
        }
        // Now read the string backwards
        --bufferIndex;
        while( bufferIndex >= 0 )
        {
            // YOU WERE HERE, ABOUT TO READ THE WATERING TIMES BACKWARDS

            --bufferIndex;
        }
        
        while( currentTimeIndex < MAX_NUMBER_OF_WATERING_TIMES )
        {
            ++currentTimeIndex;
            globalDataPtr->wateringTimes[currentTimeIndex] = -1; // Unused watering time
        }
    }
    else if( currentSetting == e_wateringDuration )
    {
        
    }

    return 0;
}
