#include "system.hpp"

typedef struct {
    uint8_t pin;
    absolute_time_t buttonPressTimestamps[SPAM_PRESS_COUNT];
    uint8_t buttonPressTsIndex;
    bool previousButtonState;
} t_button;

static t_button m_leftButton;
static t_button m_rightButton;

static void m_initialiseInputs( void );
static void m_setupButton( t_button* buttonPtr, uint8_t buttonPin );
static void m_checkSystemInputs( t_globalData* globalDataPtr );
static t_pendingInput m_checkButton( t_button* buttonPtr );
static void m_printStateChange( t_systemState from, t_systemState to );

void system_setState( t_globalData* globalDataPtr, t_systemState state )
{
    // Print state changes if DEBUG_VERBOSE is enabled
    m_printStateChange( globalDataPtr->systemState, state );

    globalDataPtr->systemState = state;

    // Run the init function for the required state
    switch( state )
    {
        case e_systemState_init:
        {
            smInit_init( globalDataPtr );
        }
        break;
        case e_systemState_idle:
        {
            
        }
        break;
        case e_systemState_info:
        {

        }
        break;
        case e_systemState_watering:
        {

        }
        break;
        case e_systemState_notSet:
        {
            // Do nothing, this should never happen
            printf( "Failed to set system state to \"notSet\"" );
        }
    }
}

void system_run( t_globalData* globalDataPtr )
{
    m_initialiseInputs();

    absolute_time_t loopEndTime;

    // Run the program
    while( true )
    {
        // Setup a timeout
        loopEndTime = make_timeout_time_ms( MAIN_LOOP_TIME_PERIOD_MS );

        // Check for button input
        m_checkSystemInputs( globalDataPtr );

        // Update the current state
        switch( globalDataPtr->systemState )
        {
            case e_systemState_init:
            {
                smInit_update( globalDataPtr );
            }
            break;
            case e_systemState_idle:
            {

            }
            break;
            case e_systemState_info:
            {

            }
            break;
            case e_systemState_watering:
            {

            }
            break;
            default:
            {
                // Unknown state, set to idle
                system_setState( globalDataPtr, e_systemState_idle );
                printf( "Unknown state in system_run\n" );
            }
            break;
        }

        // Wait until the end of the loop
        sleep_until( loopEndTime );
    }
}

static void m_initialiseInputs( void )
{
    m_setupButton( &m_leftButton, LEFT_BUTTON_PIN );
    m_setupButton( &m_rightButton, RIGHT_BUTTON_PIN );
}

static void m_setupButton( t_button* buttonPtr, uint8_t buttonPin )
{
    buttonPtr->pin = buttonPin;
    gpio_init( buttonPtr->pin );
    gpio_set_dir( buttonPtr->pin, GPIO_IN );
    for( uint8_t index = 0U; index < SPAM_PRESS_COUNT; index++ )
    {
        buttonPtr->buttonPressTimestamps[index] = nil_time;
    }
    buttonPtr->buttonPressTsIndex = 0U;
    buttonPtr->previousButtonState = false;
}

// Check for button inputs
static void m_checkSystemInputs( t_globalData* globalDataPtr )
{
    globalDataPtr->leftButtonPendingInput = m_checkButton( &m_leftButton );
    globalDataPtr->rightButtonPendingInput = m_checkButton( &m_rightButton );
}

static t_pendingInput m_checkButton( t_button* buttonPtr )
{
    t_pendingInput pendingInput;

    if( gpio_get( buttonPtr->pin ) )
    {
        if( buttonPtr->previousButtonState == false )
        {
            buttonPtr->previousButtonState = true;

            pendingInput = e_pendingInput_singlePress;

            // Log the button press timestamp and check for a spam press
            buttonPtr->buttonPressTimestamps[buttonPtr->buttonPressTsIndex] = get_absolute_time();
            absolute_time_t mostRecentButtonPressTimestamp = buttonPtr->buttonPressTimestamps[buttonPtr->buttonPressTsIndex];
            
            ++buttonPtr->buttonPressTsIndex;
            if( buttonPtr->buttonPressTsIndex >= SPAM_PRESS_COUNT )
                buttonPtr->buttonPressTsIndex = 0U;
            
            absolute_time_t previousButtonPressTimestamp = buttonPtr->buttonPressTimestamps[buttonPtr->buttonPressTsIndex];
            int64_t timeDiffUs = absolute_time_diff_us( previousButtonPressTimestamp, mostRecentButtonPressTimestamp );
            if( timeDiffUs < ( SPAM_PRESS_TIME_LIMIT_MS * 1000LL ) )
            {
                pendingInput = e_pendingInput_spamPress;
            }
        }
        else
        {
            pendingInput = e_pendingInput_none;
        }
    }
    else
    {
        buttonPtr->previousButtonState = false;
        pendingInput = e_pendingInput_none;
    }

    return pendingInput;
}

static void m_printStateChange( t_systemState from, t_systemState to )
{
#if defined(DEBUG_VERBOSE) && (DEBUG_VERBOSE==1)
    char textBuffer1[15];
    char textBuffer2[15];
    switch( from )
    {
        case e_systemState_init:
        {
            snprintf( textBuffer1, sizeof( textBuffer1 ), "INIT" );
        }
        break;
        case e_systemState_idle:
        {
            snprintf( textBuffer1, sizeof( textBuffer1 ), "IDLE" );
        }
        break;
        case e_systemState_info:
        {
            snprintf( textBuffer1, sizeof( textBuffer1 ), "INFO" );
        }
        break;
        case e_systemState_watering:
        {
            snprintf( textBuffer1, sizeof( textBuffer1 ), "WATERING" );
        }
        break;
        default:
        {
            snprintf( textBuffer1, sizeof( textBuffer1 ), "UNKNOWN" );
        }
        break;
    }
    switch( to )
    {
        case e_systemState_init:
        {
            snprintf( textBuffer2, sizeof( textBuffer2 ), "INIT" );
        }
        break;
        case e_systemState_idle:
        {
            snprintf( textBuffer2, sizeof( textBuffer2 ), "IDLE" );
        }
        break;
        case e_systemState_info:
        {
            snprintf( textBuffer2, sizeof( textBuffer2 ), "INFO" );
        }
        break;
        case e_systemState_watering:
        {
            snprintf( textBuffer2, sizeof( textBuffer2 ), "WATERING" );
        }
        break;
        default:
        {
            snprintf( textBuffer2, sizeof( textBuffer2 ), "UNKNOWN" );
        }
        break;
    }
    printf( "State changed from %s to %s\n", textBuffer1, textBuffer2 );
#endif
}
