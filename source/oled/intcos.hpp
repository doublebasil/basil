#ifndef INTCOS_HPP
#define INTCOS_HPP

#include <stdint.h>

/*
 * Function: intsin
 * --------------------
 * Calculate sin using intcos
 *
 * angle: Angle in degrees
 *
 * returns: round( 1000 * cos( angle ) )
 */
int16_t intsin( int16_t angle );

/*
 * Function: intcos
 * --------------------
 * Calculate cos using a lookup table and integer math. Uses no floats/doubles
 *
 * angle: Angle in degrees
 *
 * returns: round( 1000 * sin( angle ) )
 */
int16_t intcos( int16_t angle );

#endif // INTCOS_HPP