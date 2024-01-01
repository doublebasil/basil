#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "pico/stdlib.h"

#include "sd_card.h"
#include "ff.h"

#include "global.hpp"

int settings_readFromSDCard( t_globalData* globalDataPtr );

// int settings_writeToSDCard(); // TODO

#endif
