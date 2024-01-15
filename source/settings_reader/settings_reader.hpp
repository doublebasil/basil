#ifndef SETTINGS_READER_HPP
#define SETTINGS_READER_HPP

#include "pico/stdlib.h"

#include "sd_card.h"
#include "ff.h"

#include "settings.hpp"

int settings_readFromSDCard( t_globalData* globalDataPtr );

int settings_writeToSDCard( t_globalData* globalDataPtr );

#endif
