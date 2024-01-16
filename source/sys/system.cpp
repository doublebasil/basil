#include "system.hpp"

typedef struct {
    uint8_t pin;
    absolute_time_t buttonPressTimestamps[SPAM_PRESS_COUNT];
    uint8_t buttonPressTsIndex;
    bool previousButtonState;
} t_button;

static t_button leftButton;
static t_button rightButton;

static void m_setupButtons( void );
static void m_checkSystemInputs( t_globalData* globalDataPtr );
static t_pendingInput m_checkButton( t_button button );

void system_setState( t_globalData* globalDataPtr, t_systemState state )
{
    // Run the init function for the required state
    switch( state )
    {
        case e_systemState_init:
        {
            smInit_init( globalDataPtr );
            globalDataPtr->systemState = state;
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
    m_setupButtons();

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

static void m_setupButtons( void )
{
    // Left button
    leftButton.pin = LEFT_BUTTON_PIN;
    gpio_init( leftButton.pin );
    gpio_set_dir( leftButton.pin, GPIO_IN );
    for( uint8_t index = 0U; index < SPAM_PRESS_COUNT; index++ )
    {
        leftButton.buttonPressTimestamps[index] = nil_time;
    }
    leftButton.previousButtonState = false;
    leftButton.buttonPressTsIndex = 0U;
    // Right button
    rightButton.pin = RIGHT_BUTTON_PIN;
    gpio_init( rightButton.pin );
    gpio_set_dir( rightButton.pin, GPIO_IN );
    for( uint8_t index = 0U; index < SPAM_PRESS_COUNT; index++ )
    {
        rightButton.buttonPressTimestamps[index] = nil_time;
    }
    rightButton.previousButtonState = false;
    rightButton.buttonPressTsIndex = 0U;
}

// Check for button inputs
static void m_checkSystemInputs( t_globalData* globalDataPtr )
{
    globalDataPtr->leftButtonPendingInput = m_checkButton( leftButton );
    globalDataPtr->rightButtonPendingInput = m_checkButton( rightButton );
}

static t_pendingInput m_checkButton( t_button button )
{
    t_pendingInput pendingInput;

    if( gpio_get( button.pin ) )
    {
        pendingInput = e_pendingInput_singlePress;

        // Log the button press timestamp and check for a spam press
        button.buttonPressTimestamps[button.buttonPressTsIndex] = get_absolute_time();
        absolute_time_t mostRecentButtonPressTimestamp = button.buttonPressTimestamps[button.buttonPressTsIndex];
        
        ++button.buttonPressTsIndex;
        if( button.buttonPressTsIndex >= SPAM_PRESS_COUNT )
            button.buttonPressTsIndex = 0U;
        
        absolute_time_t previousButtonPressTimestamp = button.buttonPressTimestamps[button.buttonPressTsIndex];
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

    return pendingInput;
}
