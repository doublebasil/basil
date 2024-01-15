#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "settings.hpp"
#include "sm_init.hpp"

void system_checkInput( t_globalData* globalDataPtr );

void system_setState( t_globalData* globalDataPtr, t_systemState state );

#endif // defined SYSTEM_HPP
