#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "settings.hpp"
#include "sm_init.hpp"
#include "sm_idle.hpp"
#include "sm_wifi.hpp"

void system_setState( t_globalData* globalDataPtr, t_systemState state );

void system_run( t_globalData* globalDataPtr );

#endif // defined SYSTEM_HPP
