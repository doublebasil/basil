/* TODO: Make the reading and writing more extensible, so that instead of using custom
 * for loops for each function, you could have m_readCommaSeparatedIntegers or m_readInteger
 * 
 * TODO: Make the error codes more useful, maybe by adding an enum. Add printf to every
 * error return for debugging
 */

#include "settings.hpp"

#include "stdio.h"
#include <string.h>

#define SD_CARD_READ_BUFFER_SIZE    ( 100 )
#define CURRENT_SETTING_BUFFER_SIZE ( 50 )

#define SD_CARD_WRITE_BUFFER_SIZE   ( 100 )

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
static inline bool m_charIsNumber( char c );

int settings_readFromSDCard( t_globalData* globalDataPtr )
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    char buf[SD_CARD_READ_BUFFER_SIZE];
    const char filename[] = "settings.txt";

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
    {
        f_unmount( "0:" );
        return 2;
    }
    
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

int settings_writeToSDCard( t_globalData* globalDataPtr )
{
    // Don't write the settings if reading the SD card failed
    if( globalDataPtr->settingsReadOk == false )
        return 100;

    FRESULT fr;
    FATFS fs;
    FIL fil;
    const char filename[] = "settings.txt";

    char textBuffer[SD_CARD_WRITE_BUFFER_SIZE];
    // Create a temporary text buffer
    char tempTextBuffer[SD_CARD_WRITE_BUFFER_SIZE];

    // Mount the SD card
    fr = f_mount( &fs, "0:", 1 );
    if( fr != FR_OK )
        return 1;

    // Open the settings file
    fr = f_open( &fil, filename, FA_WRITE | FA_CREATE_ALWAYS );
    if( fr != FR_OK )
    {   
        f_unmount( "0:" );
        return 2;
    }
    
    // Note: f_printf returns the number of characters it wrote on sucess, or negative on fail
    // Note: remember to put \r\n at the end of each line

    snprintf( textBuffer, sizeof( textBuffer ), "WIFI SSID: \"%s\"\r\n", globalDataPtr->wifiSsid );
    if( ( f_printf( &fil, textBuffer ) < 0 ) )
        return 3;
    
    snprintf( textBuffer, sizeof( textBuffer ), "WIFI PASSWORD: \"%s\"\r\n\r\n", globalDataPtr->wifiPassword );
    if( ( f_printf( &fil, textBuffer ) < 0 ) )
        return 4;
    
    snprintf( textBuffer, sizeof( textBuffer ), "NOTE WATERING TIMES MUST BE 4 DIGIT MILIRARY TIME COMMA DELIMITED WITH NO SPACE\r\n" );
    if( ( f_printf( &fil, textBuffer ) < 0 ) )
        return 5;

    // Turn the watering times from seconds from midnight to military time, and into a string
    bool isTextBufferEmpty = true;
    uint16_t militaryTime;
    int32_t remainingTime;
    uint8_t stringLength = 0U;
    for( uint8_t i = 0U; i < MAX_NUMBER_OF_WATERING_TIMES; i++ )
    {
        if( globalDataPtr->wateringTimes[i] >= 0 )
        {
            // Convert time to military
            militaryTime = 0U;
            remainingTime = globalDataPtr->wateringTimes[i];
            while( remainingTime >= ( 10LL * 60LL * 60LL ) ) // Tens of hours
            {
                remainingTime -= 10LL * 60LL * 60LL;
                militaryTime += 1000;
            }
            while( remainingTime >= ( 60LL * 60LL ) ) // Hours
            {
                remainingTime -= 60LL * 60LL;
                militaryTime += 100;
            }
            while( remainingTime >= ( 10LL * 60LL ) ) // Tens of minutes
            {
                remainingTime -= 10LL * 60LL;
                militaryTime += 10;
            }
            while( remainingTime >= ( 60LL ) ) // Minutes
            {
                remainingTime -= 60LL;
                militaryTime += 1;
            }

            if( isTextBufferEmpty == true )
            {
                snprintf( textBuffer, sizeof(textBuffer), "%04d", militaryTime );
                isTextBufferEmpty = false;
                stringLength += 4;
            }
            else
            {
                // Copy from textBuffer into the temporary buffer
                strncpy( tempTextBuffer, textBuffer, SD_CARD_WRITE_BUFFER_SIZE );
                // Format the temp buffer back into text buffer
                snprintf( textBuffer, sizeof(textBuffer), "%s,%04d", tempTextBuffer, militaryTime );
                stringLength += 5;
            }
            if( stringLength >= sizeof(textBuffer) )
            {
                break;
            }
        }
        printf(textBuffer);
        printf("\n");
    }

    strncpy( tempTextBuffer, textBuffer, SD_CARD_WRITE_BUFFER_SIZE );
    snprintf( textBuffer, sizeof( textBuffer ), "WATERING TIMES: \"%s\"\r\n", tempTextBuffer );
    if( ( f_printf( &fil, textBuffer ) < 0 ) )
        return 6;

    snprintf( textBuffer, sizeof( textBuffer ), "WATERING DURATION MS: \"%d\"\r\n", globalDataPtr->wateringDurationMs );
    if( ( f_printf( &fil, textBuffer ) < 0 ) )
        return 7;

    fr = f_close( &fil );
    if( fr != FR_OK )
    {
        f_unmount( "0:" );
        return 8;
    }

    // Unmount the SD card
    f_unmount( "0:" );

    return 0;
}

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
        // Should be in 4 digit military time, comma delimited
        // Convert 24 hour time into minutes since midnight 

        // Find the end of the string
        int16_t bufferIndex;
        for( bufferIndex = 0; bufferIndex < CURRENT_SETTING_BUFFER_SIZE; bufferIndex++ )
        {
            if( settingsBuffer[bufferIndex] == 0 )
                break;
        }
        // Now read the string backwards
        uint8_t wateringTimesIndex = 0U;
        int32_t wateringTimes[MAX_NUMBER_OF_WATERING_TIMES];
        // Initialise all values of wateringTimes with -1
        for( uint8_t index = 0U; index < MAX_NUMBER_OF_WATERING_TIMES; index++ )
            wateringTimes[index] = -1;
        
        bool hasFoundValue = false;
        --bufferIndex; // Move index back onto a value
        while( bufferIndex >= 3 )
        {
            // Check the next (in reverse) 4 characters are numbers
            if( ( m_charIsNumber( settingsBuffer[bufferIndex] ) == false ) ||
                ( m_charIsNumber( settingsBuffer[bufferIndex - 1] == false ) ) ||
                ( m_charIsNumber( settingsBuffer[bufferIndex - 2] ==false ) ) ||
                ( m_charIsNumber( settingsBuffer[bufferIndex - 3] ==false ) ) )
            {
                return 1; // A character was not a number
            }

            hasFoundValue = true;
            wateringTimes[wateringTimesIndex] = ( ( (int32_t) settingsBuffer[bufferIndex] - 48) * 60 ) +
                                                ( ( (int32_t) settingsBuffer[bufferIndex - 1] - 48 ) * 10 * 60 ) +
                                                ( ( (int32_t) settingsBuffer[bufferIndex - 2] - 48 ) * 60 * 60 ) +
                                                ( ( (int32_t) settingsBuffer[bufferIndex - 3] - 48 ) * 10 * 60 * 60 );

            ++wateringTimesIndex;
            if( wateringTimesIndex == MAX_NUMBER_OF_WATERING_TIMES )
                break;

            bufferIndex -= 5;
        }

        if( hasFoundValue == false )
        {
            // No watering times
            return 1;
        }

        // Now move values from wateringTimes into globalDataPtr->wateringTimes, in ascendinging order
        int32_t currentMinimum;
        uint8_t minimumValueIndex;
        wateringTimesIndex = 0U;
        for( uint8_t loop = 0U; loop < MAX_NUMBER_OF_WATERING_TIMES; loop++ )
        {
            currentMinimum = -1;

            for( uint8_t index = 0U; index < MAX_NUMBER_OF_WATERING_TIMES; index++ )
            {
                if( wateringTimes[index] >= 0 )
                {
                    if( ( currentMinimum == -1 ) || ( wateringTimes[index] < currentMinimum ) )
                    {
                        currentMinimum = wateringTimes[index];
                        minimumValueIndex = index;
                    }
                }
            }

            if( currentMinimum == -1 )
                break;

            globalDataPtr->wateringTimes[wateringTimesIndex] = currentMinimum;
            wateringTimes[minimumValueIndex] = -1;
            ++wateringTimesIndex;
        }
        // Fill the rest of the values with -1
        while( wateringTimesIndex < MAX_NUMBER_OF_WATERING_TIMES )
        {
            globalDataPtr->wateringTimes[wateringTimesIndex] = -1;
            ++wateringTimesIndex;
        }
    }
    else if( currentSetting == e_wateringDuration )
    {
        // Find the end of the string
        uint8_t bufferIndex = 0U;
        while( settingsBuffer[bufferIndex] != 0 )
        {
            ++bufferIndex;
            if( bufferIndex >= CURRENT_SETTING_BUFFER_SIZE )
                return 1;
        }
        if( bufferIndex == 0 )
            return 1;

        // Scroll back a character
        --bufferIndex;
        // Do stuff
        uint16_t powerOfTen = 1U;
        uint16_t runningCount = 0U;
        while( true )
        {
            if( m_charIsNumber( settingsBuffer[bufferIndex] ) == false )
                return 1;

            runningCount += powerOfTen * ( settingsBuffer[bufferIndex] - 48 );
            powerOfTen *= 10;

            if( bufferIndex == 0U )
                break;
            
            --bufferIndex;
        }

        globalDataPtr->wateringDurationMs = runningCount;
    }

    return 0;
}

static inline bool m_charIsNumber( char c )
{
    if( ( c >= '0' ) && ( c <= '9' ) )
        return true;
    else
        return false;
}
