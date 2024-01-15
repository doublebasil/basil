#include "system.hpp"

static const uint8_t m_buttonPin = INPUT_BUTTON_PIN;

void system_checkInput( t_globalData* globalDataPtr )
{

}

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
