/**
  ******************************************************************************
  *
  * Copyright (c) 2014 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef FONT_COMMON_H
#define FONT_COMMON_H

#include <stdint.h>

typedef struct
{
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;
} tFontTable;

#endif /* FONT_COMMON_H */    
