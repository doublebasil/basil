#include "intcos.hpp"

static const int16_t cosLookupTable[91] = {
	1000, 1000, 999, 999, 
	998, 996, 995, 993, 
	990, 988, 985, 982, 
	978, 974, 970, 966, 
	961, 956, 951, 946, 
	940, 934, 927, 921, 
	914, 906, 899, 891, 
	883, 875, 866, 857, 
	848, 839, 829, 819, 
	809, 799, 788, 777, 
	766, 755, 743, 731, 
	719, 707, 695, 682, 
	669, 656, 643, 629, 
	616, 602, 588, 574, 
	559, 545, 530, 515, 
	500, 485, 469, 454, 
	438, 423, 407, 391, 
	375, 358, 342, 326, 
	309, 292, 276, 259, 
	242, 225, 208, 191, 
	174, 156, 139, 122, 
	105, 87, 70, 52, 
	35, 17, 0};

int16_t intsin( int16_t angle )
{
    return intcos( angle - 90 );
}

int16_t intcos( int16_t angle )
{
    // Take abs of angle
    int16_t newAngle = ( angle < 0 ) ? ( -1 * angle ) : angle;
    // Move the angle to within 0 and 360
    newAngle = newAngle - ( ( newAngle / 360 ) * 360 );

    int8_t quadrant = newAngle / 90;
    if( quadrant == 0 ) // ( newAngle >= 0 ) and ( newAngle < 90 )
        return cosLookupTable[newAngle];
    else if( quadrant == 1 ) // ( newAngle >= 90 ) and ( newAngle < 180 )
        return -1 * cosLookupTable[180 - newAngle];
    else if( quadrant == 2 ) // ( newAngle >= 180 ) and ( newAngle < 270 )
        return -1 * cosLookupTable[newAngle - 180];
    else // quadrant = 3, ( newAngle >= 270 ) and ( newAngle < 360 )
        return cosLookupTable[360 - newAngle];
}