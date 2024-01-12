// Part of main

// oled_sdWriteImage( "notification_img.txt", 20, 20 );

    /*
    // // Show off by reading some of the SD card settings
    // snprintf( m_textBuffer, sizeof(m_textBuffer), "->SSID=%s", g_globalData.wifiSsid );
    // oled_terminalWrite( m_textBuffer );
    // uint8_t wateringTimes = 0U;
    // for( uint8_t index = 0; index < MAX_NUMBER_OF_WATERING_TIMES; index++ )
    // {
    //     if( g_globalData.wateringTimes[index] != -1 )
    //         ++wateringTimes;
    // }
    // snprintf( m_textBuffer, sizeof(m_textBuffer), "->Water %d times", wateringTimes );
    // oled_terminalWrite( m_textBuffer );
    // oled_terminalWrite( "  per day" );
    // snprintf( m_textBuffer, sizeof(m_textBuffer), "->Water for %d", g_globalData.wateringDurationMs );
    // oled_terminalWrite( m_textBuffer );
    // oled_terminalWrite( "  milliseconds" );



    // if( settings_readFromSDCard( &g_globalData ) == 0 )
    // {
    //     m_settings.settingsReadOk = true;
    //     oled_terminalWrite( "Read complete!" );
    //     // SHOW OFF BY WRITING SOME OF THE SETTINGS
    // }
    // else
    // {
    //     m_settings.settingsReadOk = false;
    //     oled_terminalWrite( "Read failed" );
    // }

    // // Connect to WiFi
    // oled_terminalWrite( "Connecting to SSID" );
    // snprintf( m_textBuffer, sizeof(m_textBuffer), "\"%s\"", WIFI_SSID );
    // oled_terminalWrite( m_textBuffer );

    // cyw43_arch_wifi_connect_timeout_ms( WIFI_SSID,
    //     WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000 );
    */

// ------ Part of the old main loop

// /* NO SD CARD LOOP */
// static void m_mainLoopNoSdCard( void )
// {
//     g_globalData.nextWateringTs = make_timeout_time_ms( ( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL ) / 2UL );
//     g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );

//     while( true ) // Infinite loop
//     {
//         /* Need to perform checks in order of importance */

//         // If button is pressed and not yet in info mode, print loads of stuff
//         if( ( gpio_get( INPUT_BUTTON_PIN ) == true ) &&
//             ( g_globalData.systemState != e_systemState_info ) && 
//             ( g_globalData.systemState != e_systemState_startWatering ) )
//         {
//             // Display info
//             m_clearScreen();
//             g_globalData.systemState = e_systemState_info;
//             oled_terminalInit( TERMINAL_FONT_SIZE, TERMINAL_ERROR_COLOUR );
//             oled_terminalWrite( "NO SD CARD MODE" );
//             if( g_globalData.tankState == e_tankState_dry )
//             {
//                 oled_terminalWrite( "TANK IS DRY!" );
//             }
//             if( g_globalData.tankState == e_tankState_ok )
//             {
//                 oled_terminalWrite( "Tank contains water" );
//             }
//             else
//             {
//                 oled_terminalWrite( "Tank status unknown" );
//             }
//             oled_terminalWrite( "Next watering in:" );
//             m_timeToString( &m_textBuffer[0], sizeof( m_textBuffer ), g_globalData.nextWateringTs );
//             oled_terminalWrite( m_textBuffer );
//             oled_terminalWrite( "Hold to water now" );
//             oled_terminalSetLine( 6 );
//             m_terminalLoadingBar( &m_textBuffer[0], sizeof( m_textBuffer ), 0 );
//             oled_terminalWrite( m_textBuffer );
//             // Set screen timeout
//             g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
//         }
//         // If button pressed but already in info mode, allow for a long press
//         if( ( gpio_get( INPUT_BUTTON_PIN ) == true ) &&
//             ( g_globalData.systemState == e_systemState_info ) && 
//             ( g_globalData.systemState != e_systemState_startWatering ) )
//         {
//             absolute_time_t buttonPressStartTime = get_absolute_time();
//             bool longPress = false;
//             int64_t msSinceButtonPressed;
//             uint8_t progress;
//             while( gpio_get( INPUT_BUTTON_PIN ) )
//             {
//                 // Update the next watering time
//                 oled_terminalSetLine( 3 );
//                 m_timeToString( &m_textBuffer[0], sizeof( m_textBuffer ), g_globalData.nextWateringTs );
//                 oled_terminalWrite( m_textBuffer );
//                 // Update the long press loading bar thing
//                 msSinceButtonPressed = absolute_time_diff_us( buttonPressStartTime, get_absolute_time() ) / 1000LL;
//                 progress = (uint8_t) ( ( msSinceButtonPressed * 100LL ) / BUTTON_LONG_PRESS_TIME_MS );
//                 oled_terminalSetLine( 6 );
//                 m_terminalLoadingBar( &m_textBuffer[0], sizeof( m_textBuffer ), progress );
//                 oled_terminalWrite( m_textBuffer );
//                 if( progress >= 100 )
//                 {
//                     longPress = true;
//                     g_globalData.systemState = e_systemState_startWatering;
//                     break;
//                 }
//                 sleep_ms( 20 );
//             }
//             if( longPress == false )
//             {
//                 // Set screen timeout
//                 g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
//             }
//         }
//         // If button not pressed but still in info mode, update the watering time
//         // and check if the screen should have timed out
//         else if( g_globalData.systemState == e_systemState_info )
//         {
//             // Check for timeout
//             if( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) < 0 )
//             {
//                 // Clear screen due to timeout
//                 m_clearScreen();
//                 g_globalData.systemState = e_systemState_idle;
//             }
//             else
//             {
//                 // Update screen as it has not timed out
//                 oled_terminalSetLine( 3 );
//                 m_timeToString( &m_textBuffer[0], sizeof( m_textBuffer ), g_globalData.nextWateringTs );
//                 oled_terminalWrite( m_textBuffer );
//                 oled_terminalSetLine( 6 );
//                 m_terminalLoadingBar( &m_textBuffer[0], sizeof( m_textBuffer ), 0 );
//                 oled_terminalWrite( m_textBuffer );
//             }
//         }
//         // If the system is still on the init screen, check if it should time out
//         else if( g_globalData.systemState == e_systemState_init )
//         {
//             if( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) < 0 )
//             {
//                 g_globalData.systemState = e_systemState_idle;
//                 m_clearScreen();
//             }
//         }
//         // Check if it's time to water
//         else if( ( g_globalData.systemState == e_systemState_startWatering ) || 
//                  ( ( absolute_time_diff_us( get_absolute_time(), g_globalData.nextWateringTs ) / 1000LL ) < 0LL ) )
//         {
//             // Clear screen ready for the pump running screen
//             m_clearScreen();
//             g_globalData.systemState = e_systemState_startWatering;
//             // Run the pump
//             pump_run();
//             // Set screen state and screen timeout
//             g_globalData.systemState = e_systemState_postWatering;
//             g_globalData.screenTimeoutTs = make_timeout_time_ms( SCREEN_TIMEOUT_MS );
//             // Set the next watering time
//             g_globalData.nextWateringTs = make_timeout_time_ms( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL );
//         }
//         // Check if the post watering screen is being displayed and has timed out
//         else if( g_globalData.systemState == e_systemState_postWatering )
//         {
//             if( ( absolute_time_diff_us( get_absolute_time(), g_globalData.screenTimeoutTs ) / 1000LL ) < 0LL )
//             {
//                 m_clearScreen();
//                 g_globalData.systemState = e_systemState_idle;
//             }
//         }

//         sleep_ms( 50 );

//         // // Check in order of importance
//         // if( g_globalData.flag_displayInfo == true )
//         // {
//         //     if( g_globalData.state_display != e_displayState_infoTerminal )
//         //     {
//         //         // Clear the display
//         //         m_clearScreen();
//         //         // Set new display status
//         //         g_globalData.state_display = e_displayState_infoTerminal;
//         //         g_globalData.screenChangeTimestamp = get_absolute_time();
//         //         // Create a terminal and write the info
//         //         oled_terminalInit( 12, TERMINAL_NORMAL_COLOUR );
//         //         //                  "MAX TERMINAL WIDTH"
//         //         oled_terminalWrite( "No SD card present" );
//         //         oled_terminalWrite( "Next watering in:" );
//         //         char text[19];
//         //         m_timeUntil( &text[0], sizeof(text), g_globalData.nextWateringTimestamp );
//         //         oled_terminalWrite( text );
//         //     }

//         //     // bool longPressActivated = false;
//         //     // while( gpio_get( INPUT_BUTTON_PIN ) )
//         //     // {

//         //     // }

//         // }
//         // if( g_globalData.flag_watering == true )
//         // {
//         //     // Clear the display


//         //     // Run the pump
//         //     pump_run();

//         //     // Clear the watering flag
//         //     g_globalData.flag_watering = false;
//         //     // If a displayInfo flag was raised, it's now not relevant so clear
//         //     g_globalData.flag_displayInfo = false;

//         //     // Setup a new alarm
//         //     cancel_alarm( g_globalData.alarm_watering );
//         //     g_globalData.nextWateringTimestamp = make_timeout_time_ms( make_timeout_time_ms( DEFAULT_WATERING_PERIOD_HOURS * 60UL * 60UL * 1000UL ) );
//         //     g_globalData.alarm_watering = add_alarm_in_ms( g_globalData.nextWateringTimestamp,
//         //                                                    m_noSdCardWateringCb, NULL, false );

//         // }
//         // else if( g_globalData.flag_displayInfo == true )
//         // {
//         //     // Display some info
//         // }

//         // The sleep function should put the board into low power mode for a short time
//         // printf( "loop\n" );
//         // sleep_ms( 50 );
//     }
// }