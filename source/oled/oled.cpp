/* --- STANDARD LIBRARY INCLUDES ---------------------------------------------- */
#include "oled.hpp"

#include <cstdlib> // For the 'free' function
#include <stdio.h> // Just for debugging
#ifdef OLED_INCLUDE_LOADING_CIRCLE
#include <math.h>
#endif // OLED_INCLUDE_LOADING_CIRCLE


/* --- PICO LIBRARY INCLUDES -------------------------------------------------- */
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

/* --- OTHER LIBRARY INCLUDES ------------------------------------------------ */
#ifdef OLED_INCLUDE_QR_GENERATOR
#include "qrcodegen.h"
#endif

#ifdef OLED_INCLUDE_LOADING_CIRCLE
#include "intcos.hpp"
#endif // defined OLED_INCLUDE_LOADING_CIRCLE

/* --- PREPROCESSOR -----------------------------------------------------------*/
#ifdef OLED_INCLUDE_FONT8
#include "font8.h"
#endif /* OLED_INCLUDE_FONT8 */
#ifdef OLED_INCLUDE_FONT12
#include "font12.h"
#endif /* OLED_INCLUDE_FONT12 */
#ifdef OLED_INCLUDE_FONT16
#include "font16.h"
#endif /* OLED_INCLUDE_FONT16 */
#ifdef OLED_INCLUDE_FONT20
#include "font20.h"
#endif /* OLED_INCLUDE_FONT20 */
#ifdef OLED_INCLUDE_FONT24
#include "font24.h"
#endif /* OLED_INCLUDE_FONT24 */

#ifdef OLED_INCLUDE_SD_IMAGES
#define OLED_SD_BUFFER_SIZE     ( 100U )
#endif // OLED_INCLUDE_SD_IMAGES

/* --- MODULE SCOPE VARIABLES ------------------------------------------------- */
static int8_t m_csPin;
static int8_t m_dcPin;
static int8_t m_rstPin;
static int8_t m_spiInstance;
static uint8_t m_displayWidth;
static uint8_t m_displayHeight;

/* --- LOADING BAR RELATED MODULE SCOPE VARIABLES --- */
#if defined OLED_INCLUDE_LOADING_BAR_HORIZONTAL || defined OLED_INCLUDE_LOADING_CIRCLE
// Common to both loading bars
static uint8_t* m_loadingBarBitmapPtr1 = NULL;
static uint8_t* m_loadingBarBitmapPtr2 = NULL;
static uint16_t m_loadingBarCallocSize;
static uint16_t m_loadingBarColour;
typedef enum
{
    e_loadingBarStateUninitialised,
    e_loadingBarStateHorizontal,
    e_loadingBarStateCircle,
} t_loadingBarState;
typedef enum
{
    e_loadingBarBitmap1Next,
    e_loadingBarBitmap2Next,
} t_loadingBarBitmapNext;
static t_loadingBarState m_loadingBarState = e_loadingBarStateUninitialised;
static t_loadingBarBitmapNext m_loadingBarBitmapNext;
#endif // defined OLED_INCLUDE_LOADING_BAR_HORIZONTAL || defined OLED_INCLUDE_LOADING_CIRCLE
#ifdef OLED_INCLUDE_LOADING_BAR_HORIZONTAL
static uint8_t m_loadingBarHorizontalTopLeftX;
static uint8_t m_loadingBarHorizontalTopLeftY;
static uint8_t m_loadingBarHorizontalBottomRightX;
static uint8_t m_loadingBarHorizontalBottomRightY;
#endif // defined OLED_INCLUDE_LOADING_BAR_HORIZONTAL
#ifdef OLED_INCLUDE_LOADING_CIRCLE
static uint8_t m_loadingCircleCenterX;
static uint8_t m_loadingCircleCenterY;
static uint8_t m_loadingCircleBitmapWidth;
static uint8_t m_loadingCircleOuterRadius;
static uint8_t m_loadingCircleInnerRadius;
#endif // defined OLED_INCLUDE_LOADING_CIRCLE

/* --- FONT RELATED MODULE SCOPE VARIABLES --- */
#if defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24
static uint8_t* m_terminalBitmapPtr1 = NULL;
static uint8_t* m_terminalBitmapPtr2 = NULL;
typedef enum
{
    e_terminalUninitialised,
    e_terminalBitmap1Next,
    e_terminalBitmap2Next,
} t_terminalBitmapState;
static t_terminalBitmapState m_terminalBitmapState = e_terminalUninitialised;
static uint8_t m_terminalFontSize;
static uint16_t m_terminalFontColour;
static uint8_t m_terminalCurrentLine;
static tFontTable* m_terminalFontTablePtr;
static uint8_t m_terminalBitmapBytesPerRow;
static uint16_t m_terminalBitmapCallocSize;
static bool m_terminalIsLineTemp;
static uint8_t m_terminalHeightInLines;
#endif // defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24


/* --- MODULE SCOPE FUNCTION PROTOTYPES --------------------------------------- */
static inline void m_displayInit( void );
static inline void m_chipSelect( void );
static inline void m_chipDeselect( void );
static inline void m_writeReg( uint8_t reg );
static inline void m_writeData( uint8_t data );

/* --- FONT RELATED MODULE SCOPE FUNCTIONS --- */
#if defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24
static void m_terminalPushBitmap( void );
static inline void m_terminalWriteChar( char character, uint8_t textOriginX, uint8_t textOriginY );
static inline void m_terminalWrite( const char text[] );
#endif // defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24

/* --- LOADING CIRCLE MODULE SCOPE FUNCTIONS --- */
#ifdef OLED_INCLUDE_LOADING_CIRCLE
static inline void m_loadingCircleProcessQuadrant( uint8_t* bitmapPtr, uint8_t xLowerBound,
    uint8_t xUpperBound, bool yIsPositive, uint8_t angle );
static inline void m_loadingCircleSetBitmap( uint8_t* bitmapPtr, uint8_t x, uint8_t y, bool val );
#endif // OLED_INCLUDE_LOADING_CIRCLE

/* --- PUBLIC FUNCTION IMPLEMENTATIONS ---------------------------------------- */

// Initialise GPIO and SPI
int oled_init( int8_t dinPin, int8_t clkPin, int8_t csPin, int8_t dcPin, 
    int8_t rstPin, int8_t spiOutput, unsigned int baudrate, uint8_t displayWidth,
    uint8_t displayHeight )
{
    // Set module variables
    m_csPin = csPin;
    m_dcPin = dcPin;
    m_rstPin = rstPin;
    m_displayWidth = displayWidth;
    m_displayHeight = displayHeight;

    // Set cs, dc and rst pins to be GPIO output pins
    gpio_init( m_csPin );
    gpio_init( m_dcPin );
    gpio_init( m_rstPin );
    gpio_set_dir( m_csPin, GPIO_OUT );
    gpio_set_dir( m_dcPin, GPIO_OUT );
    gpio_set_dir( m_rstPin, GPIO_OUT );
    // Set cs, dc and rst pins to high output
    gpio_put( m_csPin, 1 );
    gpio_put( m_dcPin, 1 );
    gpio_put( m_rstPin, 1 );

    // Set the SPI pins
    gpio_set_function( clkPin, GPIO_FUNC_SPI );
    gpio_set_function( dinPin, GPIO_FUNC_SPI );

    // Check din and clk pins are suitable, and init SPI at specified baud rate
    if( ( spiOutput == 0 ) &&
        ( ( dinPin == 3 ) || ( dinPin == 7 ) || ( dinPin == 19 ) ) && // Check din pin is connected to a SPI0 TX
        ( ( clkPin == 2 ) || ( clkPin == 6 ) || ( clkPin == 18 ) ) )  // Check clk pin is connected to a SPI0 SCK
    {
        spi_init( spi0, baudrate );
        m_spiInstance = 0;
        /* Set the SPI format to be the same as Arduino's SPI Mode 3, i.e.:
         * Most significant bit first, clock polarity = 1, clock phase = 1
         * This line made no difference, but I'd like to ensure correct settings */
        spi_set_format( spi0, 8U, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST );
    }
    else if( ( spiOutput == 1 ) &&
        ( ( dinPin == 11 ) || ( dinPin == 15 ) ) && // Check din pin is connected to a SPI1 TX
        ( ( clkPin == 10 ) || ( clkPin == 14 ) ) )  // Check clk pin is connected to a SPI1 SCK
    {
        spi_init( spi1, baudrate );
        // m_spiInstance is so that 
        m_spiInstance = 1;
        /* Set the SPI format to be the same as Arduino's SPI Mode 3, i.e.:
         * Most significant bit first, clock polarity = 1, clock phase = 1
         * This line made no difference, but I'd like to ensure correct settings */
        spi_set_format( spi1, 8U, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST );
    }
    else
    {
        // Invalid settings
        return 1;
    }

    m_chipSelect();

    gpio_put( m_rstPin, 1 );
    sleep_ms( 10U );
    gpio_put( m_rstPin, 0 );
    sleep_ms( 10U );
    gpio_put( m_rstPin, 1 );

    m_displayInit();

    sleep_ms( 10U );

    // Turn on the display
    m_writeReg(0xAF);

    sleep_ms( 10U );

    oled_clear();
    // CS is now inactive (high)

    return 0;
}

void oled_clear( void )
{
    m_chipSelect();
    m_writeReg(0x15);
    m_writeData(0);
    m_writeData(127);
    m_writeReg(0x75);
    m_writeData(0);
    m_writeData(127);

    m_writeReg(0x5C);

    for( uint16_t i = 0; i < m_displayWidth * m_displayHeight; ++i )
    {
        m_writeData( 0x00 );
        m_writeData( 0x00 );
    }
    m_chipDeselect();
}

void oled_deinitAll( void )
{
    oled_terminalDeinit();
    oled_loadingBarDeinit();
    oled_loadingCircleDeinit();
}

void oled_setPixel( uint8_t x, uint8_t y, uint16_t colour )
{
    if( ( x < 0U ) || ( y < 0U ) || ( x > m_displayWidth ) || ( y > m_displayHeight ) )
    {
        // Pixel is out of bounds
        return;
    }

    m_chipSelect();

    m_writeReg(0x15);
    m_writeData(x);
    m_writeData(x);
    m_writeReg(0x75);
    m_writeData(y);
    m_writeData(y);
    m_writeReg(0x5C);   
    
    // The 16 bits are sent in two bytes
    m_writeData(colour >> 8);
    m_writeData(colour);

    m_chipDeselect();
}

void oled_fill( uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t colour )
{
    uint8_t xMin;
    uint8_t xMax;
    uint8_t yMin;
    uint8_t yMax;
    if( x1 < x2 )
    {
        xMin = x1;
        xMax = x2;
    }
    else
    {
        xMin = x2;
        xMax = x1;
    }
    if( y1 < y2 )
    {
        yMin = y1;
        yMax = y2;
    }
    else
    {
        yMin = y2;
        yMax = y1;
    }
    for( uint8_t x = xMin; x <= xMax; ++x )
    {
        for( uint8_t y = yMin; y <= yMax; ++y )
        {
            oled_setPixel( x, y, colour );
        }
    }
}

void oled_drawLineBetweenPoints( uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, 
    uint16_t colour, uint8_t thickness )
{
    /* To avoid the use of floats/doubles, int32_t will be used and each coordinate
     * will be multipied by a constant */
    const int32_t scale = 1000;
    int32_t xScaled = (int32_t) x1 * scale;
    int32_t yScaled = (int32_t) y1 * scale;
    int32_t dx = (int32_t) x2 - (int32_t) x1;
    int32_t dy = (int32_t) y2 - (int32_t) y1;
    uint8_t x;
    uint8_t y;

    uint16_t stepsLimit;
    uint16_t dxAbs;
    uint16_t dyAbs;
    
    if( dx < 0 )
        dxAbs = (uint16_t) ( -1 * dx );
    else
        dxAbs = (uint16_t) dx;

    if( dy < 0 )
        dyAbs = (uint16_t) ( -1 * dy );
    else
        dyAbs = (uint16_t) dy;
    
    if( dxAbs > dyAbs )
        stepsLimit = dxAbs;
    else
        stepsLimit = dyAbs;

    int32_t xIncrementScaled = ( dx * scale ) / stepsLimit;
    int32_t yIncrementScaled = ( dy * scale ) / stepsLimit;

    for( uint16_t step = 0U; step < stepsLimit; step++ )
    {
        x = (uint8_t) ( xScaled / scale );
        y = (uint8_t) ( yScaled / scale );

        if( thickness > 0 )
        {
            for( int16_t xThicken = x - thickness; xThicken < x + thickness + 1; xThicken++ )
            {
                if( xThicken < 0 )
                    continue;
                
                for( int16_t yThicken = y - thickness; yThicken < y + thickness + 1; yThicken++ )
                {
                    if( yThicken < 0 )
                        continue;

                    oled_setPixel( (uint8_t) xThicken, (uint8_t) yThicken, colour );
                }
            }
        }
        else // don't thicken line
        {
            oled_setPixel( x, y, colour );
        }

        xScaled += xIncrementScaled;
        yScaled += yIncrementScaled;
    }
}

#ifdef OLED_INCLUDE_TEST_FUNCTION
void oled_test( void ) // Needs rewriting
{
    uint16_t color = 0;
    uint8_t drawMode = 0;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    m_chipSelect();
    for( ;; )
    {
        for( uint8_t y = 0U; y < m_displayHeight; ++y )
        {
            for( uint8_t x = 0U; x < m_displayWidth; ++x )
            {
                if( drawMode == 0 )
                    color = ( ( x / 4 ) & 0b11111 ) << ( 5 + 6 );
                    // color = ( (uint16_t) x * (uint16_t) x) + ( (uint16_t) y * (uint16_t) y);
                else if( drawMode == 1 )
                    color = ( ( y / 3 ) & 0b111111 ) << ( 5 );
                    // color = ( (uint16_t) x * (uint16_t) x) - ( (uint16_t) y * (uint16_t) y);
                else if( drawMode == 2)
                    color = ( ( x / 4 ) & 0b11111 );
                else if (drawMode == 3 )
                {
                    red = ( x / 4 ) & 0b11111;
                    green = ( ( x + y ) / 4 ) & 0b111111;
                    blue = ( y / 4 ) & 0b11111;
                    color = ( red << ( 5 + 6 ) ) + ( green << 5 ) + blue;
                }

                m_writeReg(0x15);
                m_writeData(x);
                m_writeData(x);
                m_writeReg(0x75);
                m_writeData(y);
                m_writeData(y);
                // fill!
                m_writeReg(0x5C);   
                
                // The 16 bits are sent in two seperate bytes
                m_writeData(color >> 8);
                m_writeData(color);
            }
        }
        sleep_ms( 1000U );
        ++drawMode;
        if( drawMode == 4 )
            drawMode = 0;
    }
    
}
#endif /* OLED_INCLUDE_TEST_FUNCTION */

#ifdef OLED_INCLUDE_LOADING_BAR_HORIZONTAL
int oled_loadingBarInit( uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, 
    uint16_t colour )
{
    // Ensure the loading bar isn't already initialised
    if( m_loadingBarState != e_loadingBarStateUninitialised )
        return 1;

    if( x1 < x2 )
    {
        m_loadingBarHorizontalTopLeftX = x1;
        m_loadingBarHorizontalBottomRightX = x2;
    }
    else
    {
        m_loadingBarHorizontalTopLeftX = x2;
        m_loadingBarHorizontalBottomRightX = x1;
    }

    if( y1 < y2 )
    {
        // It's top in normal cartesian, but on a display positive y is down. I'm using cartesian here
        m_loadingBarHorizontalTopLeftY = y1;
        m_loadingBarHorizontalBottomRightY = y2;
    }
    else
    {
        m_loadingBarHorizontalTopLeftY = y2;
        m_loadingBarHorizontalBottomRightY = y1;
    }

    // 1D bitmap for the horizontal loading bar, one bit per pixel
    uint8_t barWidthInPixels = m_loadingBarHorizontalBottomRightX - m_loadingBarHorizontalTopLeftX;
    m_loadingBarCallocSize = barWidthInPixels / 8U;
    // Round up if needed
    if( ( barWidthInPixels / 8U ) != 0U )
        ++m_loadingBarCallocSize;

    // We love a good calloc
    m_loadingBarBitmapPtr1 = (uint8_t*) calloc( m_loadingBarCallocSize, sizeof( uint8_t ) );
    if( m_loadingBarBitmapPtr1 == NULL )
    {
        // But not when it does this
        return 2;
    }
    // Calloc! Calloc! Calloc!
    m_loadingBarBitmapPtr2 = (uint8_t*) calloc( m_loadingBarCallocSize, sizeof( uint8_t ) );
    if( m_loadingBarBitmapPtr2 == NULL )
    {
        // Free the first bitmap before exiting
        free( m_loadingBarBitmapPtr1 );
        m_loadingBarBitmapPtr1 = NULL;
        return 2;
    }

    m_loadingBarState = e_loadingBarStateHorizontal;
    m_loadingBarBitmapNext = e_loadingBarBitmap1Next;
    m_loadingBarColour = colour;

    return 0;
}

bool oled_loadingBarIsInit( void )
{
#if defined(OLED_INCLUDE_LOADING_BAR_HORIZONTAL) || defined(OLED_INCLUDE_LOADING_CIRCLE)
    if( m_loadingBarState == e_loadingBarStateHorizontal )
        return true;
    else
        return false;
#else
    return false;
#endif
}

void oled_loadingBarDisplay( uint8_t progress ) 
{
    // Ensure the loading bar has been initialised
    if( m_loadingBarState != e_loadingBarStateHorizontal )
        return; // Horizontal loading bar has not been initialised

    uint8_t bitmapIndex = 0U;
    uint8_t* bitmapToChange = ( m_loadingBarBitmapNext == e_loadingBarBitmap1Next ) ? m_loadingBarBitmapPtr1 : m_loadingBarBitmapPtr2;
    uint8_t barWidthInPixels = m_loadingBarHorizontalBottomRightX - m_loadingBarHorizontalTopLeftX;
    // Calculate the number of pixels that should be "on"
    uint16_t remainingPixelsToFill = ( (uint16_t) barWidthInPixels * (uint16_t) progress ) / 255U;

    // Fill whole bytes in the bitmap first
    while( remainingPixelsToFill > 8U )
    {
        // Make sure we aren't writing out of bounds
        if( bitmapIndex >= m_loadingBarCallocSize ) // bitmapIndex is unsigned so don't check bitmapIndex isn't negative
            return;
        // Set all the pixels within this byte to on
        bitmapToChange[bitmapIndex] = 0b11111111U;
        remainingPixelsToFill -= 8U;
        ++bitmapIndex;
    }
    // Determine the value of the overlap byte on the loading bar, where the 'on' part of the bar ends
    uint8_t overlapByte = 0x00U;
    while( remainingPixelsToFill > 0U )
    {
        overlapByte >>= 1;
        overlapByte += 0b10000000U;
        remainingPixelsToFill -= 1U;
    }
    // Set the overlap byte (ensure we aren't writing out of bounds)
    if( bitmapIndex > m_loadingBarCallocSize )
        return;
    bitmapToChange[bitmapIndex] = overlapByte;
    ++bitmapIndex;
    // Set the rest of the bitmap to 0
    while( bitmapIndex < m_loadingBarCallocSize )
    {
        bitmapToChange[bitmapIndex] = 0x00U;
        ++bitmapIndex;
    }

    // Now to push the bitmap
    uint8_t* bitmapCurrent = ( m_loadingBarBitmapNext == e_loadingBarBitmap1Next ) ? m_loadingBarBitmapPtr2 : m_loadingBarBitmapPtr1;
    uint8_t xPixelPosition = m_loadingBarHorizontalTopLeftX; // Refers to pixel coordinates on the display
    uint8_t bitmapPixel = 0U; // Can exceed 8, refers to pixel number along the loading bar
    bitmapIndex = 0U; // Reuse this variable
    uint8_t bitmapBitmask;

    while( xPixelPosition <= m_loadingBarHorizontalBottomRightX )
    {
        bitmapIndex = bitmapPixel / 8U;
        bitmapBitmask = 0b10000000U >> ( bitmapPixel % 8U );

        // Check for a difference in the bitmaps
        if( ( ( bitmapCurrent[bitmapIndex] & bitmapBitmask ) != 0U ) && 
            ( ( bitmapToChange[bitmapIndex] & bitmapBitmask ) == 0U ) )
        {
            // Turn some pixels off
            for( uint8_t y = m_loadingBarHorizontalBottomRightY; y <= m_loadingBarHorizontalTopLeftY; y++ )
                oled_setPixel( xPixelPosition, y, 0x0000U );
        }
        else if( ( ( bitmapCurrent[bitmapIndex] & bitmapBitmask ) == 0U ) && 
            ( ( bitmapToChange[bitmapIndex] & bitmapBitmask ) != 0U ) )
        {
            // Turn some pixels on
            for( uint8_t y = m_loadingBarHorizontalBottomRightY; y <= m_loadingBarHorizontalTopLeftY; y++ )
                oled_setPixel( xPixelPosition, y, m_loadingBarColour );
        }
        // Otherwise leave display as is

        ++bitmapPixel;
        ++xPixelPosition;
    }

    // Change the next bitmap
    m_loadingBarBitmapNext = ( m_loadingBarBitmapNext == e_loadingBarBitmap1Next ) ? e_loadingBarBitmap2Next : e_loadingBarBitmap1Next;
}

#endif /* OLED_INCLUDE_LOADING_BAR_HORIZONTAL */

void oled_loadingBarDeinit( void )
{
#if defined(OLED_INCLUDE_LOADING_BAR_HORIZONTAL)
    if( m_loadingBarState != e_loadingBarStateHorizontal )
        return;

    if( m_loadingBarBitmapPtr1 != NULL )
    {
        free( m_loadingBarBitmapPtr1 );
        m_loadingBarBitmapPtr1 = NULL;
    }
    if( m_loadingBarBitmapPtr2 != NULL )
    {
        free( m_loadingBarBitmapPtr2 );
        m_loadingBarBitmapPtr2 = NULL;
    }

    m_loadingBarState = e_loadingBarStateUninitialised;
#endif
}

#ifdef OLED_INCLUDE_LOADING_CIRCLE

int oled_loadingCircleInit( uint8_t originX, uint8_t originY, uint8_t outerRadius, 
    uint8_t innerRadius, uint16_t colour ) 
{
    // Ensure the loading bar isn't already initialised
    if( m_loadingBarState != e_loadingBarStateUninitialised )
        return 1;

    // Ensure the radius values are sensible
    if( ( outerRadius < innerRadius ) || ( outerRadius - innerRadius < 2 ) ||
        ( outerRadius <= 4U ) )
        return 3;
    
    // Calculate the required calloc size
    uint16_t bitmapWidthInPixels = ( (uint16_t) outerRadius * 2U ) - 1U;
    uint16_t bitmapTotalNumberOfPixels = bitmapWidthInPixels * bitmapWidthInPixels;
    m_loadingBarCallocSize = bitmapTotalNumberOfPixels / 8U;
    // Round up if needed 
    if( bitmapTotalNumberOfPixels % 8U != 0U )
        ++m_loadingBarCallocSize;

    // Callocs
    m_loadingBarBitmapPtr1 = (uint8_t*) calloc( m_loadingBarCallocSize, sizeof( uint8_t ) );
    if( m_loadingBarBitmapPtr1 == NULL )
    {
        return 2; // Failed calloc, RIP
    }
    m_loadingBarBitmapPtr2 = (uint8_t*) calloc( m_loadingBarCallocSize, sizeof( uint8_t ) );
    if( m_loadingBarBitmapPtr2 == NULL )
    {
        // Free the first bitmap before exiting
        free( m_loadingBarBitmapPtr1 );
        m_loadingBarBitmapPtr1 = NULL;
        return 2;
    }

    m_loadingCircleCenterX = originX;
    m_loadingCircleCenterY = originY;
    m_loadingCircleInnerRadius = innerRadius;
    m_loadingCircleOuterRadius = outerRadius;
    m_loadingCircleBitmapWidth = ( m_loadingCircleOuterRadius * 2U ) - 1U;

    m_loadingBarColour = colour;
    m_loadingBarState = e_loadingBarStateCircle;
    m_loadingBarBitmapNext = e_loadingBarBitmap1Next;

    return 0;
}

bool oled_loadingCircleIsInit( void )
{
#if defined(OLED_INCLUDE_LOADING_CIRCLE) || defined(OLED_INCLUDE_LOADING_BAR_HORIZONTAL)
    if( m_loadingBarState == e_loadingBarStateCircle )
        return true;
    else
        return false;
#else
    return false;
#endif
}

void oled_loadingCircleDisplay( uint8_t progress )
{
    if( progress > 252U )
        progress = 252U;
    
    // Check the loading circle has been initialised
    if( m_loadingBarState != e_loadingBarStateCircle )
        return;

    // Get the bitmaps
    uint8_t* nextBitmap;
    uint8_t* currentBitmap;
    if( m_loadingBarBitmapNext == e_loadingBarBitmap1Next )
    {
        nextBitmap = m_loadingBarBitmapPtr1;
        currentBitmap = m_loadingBarBitmapPtr2;
    }
    else
    {
        nextBitmap = m_loadingBarBitmapPtr2;
        currentBitmap = m_loadingBarBitmapPtr1;
    }

    uint8_t progressRemaining = progress;
    uint8_t currentQuadrant = 0U; // This should be set by a parameter in the init function
    /* If I coded this well, you could change the currentQuadrant start value to make it start in a different place
     * Q3 | Q0
     * -------            Brain go brr
     * Q2 | Q1 */
    uint8_t quadrantCounter = 0U; // Need to process all 4 quadrants
    uint8_t xLowerBound;
    uint8_t xUpperBound;
    bool yIsPositive; // Positive meaning the upper part of the display (which actually has lower y values)

    while( quadrantCounter < 4U )
    {
        // These coordinates refer to the bitmap. Top left of the bitmap is 0, 0
        if( currentQuadrant == 0U )
        {
            xLowerBound = m_loadingCircleOuterRadius - 1U;
            xUpperBound = ( m_loadingCircleOuterRadius * 2U ) - 1U;
            yIsPositive = true;
        }
        else if( currentQuadrant == 1U )
        {
            xLowerBound = m_loadingCircleOuterRadius - 1U;
            xUpperBound = ( m_loadingCircleOuterRadius * 2U ) - 1U;
            yIsPositive = false;
        }
        else if( currentQuadrant == 2U )
        {
            xLowerBound = 0U;
            xUpperBound = m_loadingCircleOuterRadius;
            yIsPositive = false;
        }
        else // currentQuadrant == 3U
        {
            xLowerBound = 0U;
            xUpperBound = m_loadingCircleOuterRadius;
            yIsPositive = true;
        }

        if( progressRemaining >= 63U )
        {
            // This currentQuadrant will be completely filled

            m_loadingCircleProcessQuadrant( nextBitmap, xLowerBound, xUpperBound, yIsPositive, 90U );

            progressRemaining -= 63U;
        }
        else if( progressRemaining == 0U)
        {
            // This quarter of the bitmap will be empty

            m_loadingCircleProcessQuadrant( nextBitmap, xLowerBound, xUpperBound, yIsPositive, 0U );
        }
        else // Then progressRemaining <= 62
        {
            // This quadrant won't be completely filled

            m_loadingCircleProcessQuadrant( nextBitmap, xLowerBound, xUpperBound, yIsPositive, (uint8_t) ( ( (uint16_t) progressRemaining * 90U ) / 63U ) );
            progressRemaining = 0U;
        }

        ++quadrantCounter;
        ++currentQuadrant;
        if( currentQuadrant == 4U )
            currentQuadrant = 0U;
    }

    // Cut out the middle of the loading donut
    uint8_t triangleWidth = ( m_loadingCircleInnerRadius - 1U );
    uint8_t yLimit;
    if( m_loadingCircleInnerRadius > 0U )
    {
        for( uint8_t x = ( m_loadingCircleOuterRadius - 1 ) - ( m_loadingCircleInnerRadius - 1U ); x < ( m_loadingCircleOuterRadius - 1 ) + ( m_loadingCircleInnerRadius ); x++ )
        {
            yLimit = (uint8_t) sqrt( ( (uint16_t) m_loadingCircleInnerRadius * (uint16_t) m_loadingCircleInnerRadius ) - ( (uint16_t) triangleWidth * (uint16_t) triangleWidth ) );
            for( uint8_t y = ( m_loadingCircleOuterRadius - 1 ) - yLimit; y < ( m_loadingCircleOuterRadius - 1 ) + yLimit; y++ )
            {
                m_loadingCircleSetBitmap( nextBitmap, x, y, false );
            }
            if( x < ( m_loadingCircleOuterRadius - 1 ) )
                --triangleWidth;
            else
                ++triangleWidth;
        }
    }

    // Push the bitmap
    uint16_t bitPosition = 0U;
    uint8_t displayPositionX = m_loadingCircleCenterX - ( m_loadingCircleOuterRadius - 1U );
    uint8_t displayPositionY = m_loadingCircleCenterY - ( m_loadingCircleOuterRadius - 1U );
    const uint8_t displayLimitX = m_loadingCircleCenterX + ( m_loadingCircleOuterRadius - 1U );
    while( bitPosition < ( (uint16_t) m_loadingCircleBitmapWidth * (uint16_t) m_loadingCircleBitmapWidth ) )
    {
        if( ( ( nextBitmap[bitPosition / 8U] & ( 0b10000000 >> bitPosition % 8U ) ) == 0U ) &&
            ( ( currentBitmap[bitPosition / 8U] & ( 0b10000000 >> bitPosition % 8U ) ) != 0U ) )
        {
            oled_setPixel( displayPositionX, displayPositionY, 0x0000U );
        }
        else if( ( ( nextBitmap[bitPosition / 8U] & ( 0b10000000 >> bitPosition % 8U ) ) != 0U ) &&
            ( ( currentBitmap[bitPosition / 8U] & ( 0b10000000 >> bitPosition % 8U ) ) == 0U ) )
        {
            oled_setPixel( displayPositionX, displayPositionY, m_loadingBarColour );
        }
        // Otherwise leave display as is

        ++displayPositionX;
        if( displayPositionX > displayLimitX )
        {
            displayPositionX = m_loadingCircleCenterX - ( m_loadingCircleOuterRadius - 1U );
            ++displayPositionY;
        }

        ++bitPosition;
    }

    // Update the module scope variables
    m_loadingBarBitmapNext = ( m_loadingBarBitmapNext == e_loadingBarBitmap1Next ) ? e_loadingBarBitmap2Next : e_loadingBarBitmap1Next;
    
}

#endif /* OLED_INCLUDE_LOADING_CIRCLE */

void oled_loadingCircleDeinit( void )
{
#if defined(OLED_INCLUDE_LOADING_CIRCLE)
    if( m_loadingBarState != e_loadingBarStateCircle )
        return;

    if( m_loadingBarBitmapPtr1 != NULL )
    {
        free( m_loadingBarBitmapPtr1 );
        m_loadingBarBitmapPtr1 = NULL;
    }
    if( m_loadingBarBitmapPtr2 != NULL )
    {
        free( m_loadingBarBitmapPtr2 );
        m_loadingBarBitmapPtr2 = NULL;
    }

    m_loadingBarState = e_loadingBarStateUninitialised;
#endif
}

#if defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24
void oled_writeChar( uint8_t x, uint8_t y, char character, uint8_t fontSize, uint16_t colour )
{
    tFontTable* fontTable;

    if( ( x < 0 ) || ( x > m_displayWidth ) || ( y < 0 ) || ( y > m_displayHeight ) )
    {
        // Out of display bounds
        return;
    }

    if( ( character < 32 ) || ( character > ( 32 + 95 ) ) )
    {
        // Invalid ascii character, draw a blank character
        character = ' '; // = 32
    }

    switch( fontSize )
    {
#ifdef OLED_INCLUDE_FONT8
        case 8U:
        {
            fontTable = &Font8;
        }
        break;
#endif /* OLED_INCLUDE_FONT8 */
#ifdef OLED_INCLUDE_FONT12
        case 12U:
        {
            fontTable = &Font12;
        }
        break;
#endif /* OLED_INCLUDE_FONT12 */
#ifdef OLED_INCLUDE_FONT16
        case 16U:
        {
            fontTable = &Font16;
        }
        break;
#endif /* OLED_INCLUDE_FONT16 */
#ifdef OLED_INCLUDE_FONT20
        case 20U:
        {
            fontTable = &Font20;
        }
        break;
#endif /* OLED_INCLUDE_FONT20 */
#ifdef OLED_INCLUDE_FONT24
        case 24U:
        {
            fontTable = &Font24;
        }
        break;
#endif /* OLED_INCLUDE_FONT24 */
        default:
        {
            // Do nothing, font height not supported
            return;
        }
        break;
    }

    // Write the character

    // Calculate the width of a character in bytes
    uint16_t characterWidthBytes = ( fontTable->Width / 8U ) + 1U;
    // Find starting position in the array. Note that the first 32 characters of
    // ascii aren't human readable
    uint16_t arrayPosition = fontTable->Height * characterWidthBytes * (uint16_t) ( character - 32U );
    // bitPosition records the position within each byte
    uint8_t bitPosition;
    // Displacement variables is relative to the characters origin
    for( uint8_t yDisplacement = 0U; yDisplacement < fontTable->Height; yDisplacement++ )
    {
        bitPosition = 0;
        for( uint8_t xDisplacement = 0U; xDisplacement < fontTable->Width; xDisplacement++ )
        {
            if( ( fontTable->table[arrayPosition] & ( 0b10000000 >> bitPosition ) ) != 0 )
                oled_setPixel( x + xDisplacement, y + yDisplacement, colour );

            ++bitPosition;
            if( bitPosition == 8 )
            {
                ++arrayPosition;
                bitPosition = 0U;
            }
        }
        ++arrayPosition;
    }
}

void oled_writeText( uint8_t xStartPos, uint8_t yStartPos, const char text[],
    uint8_t fontSize, uint16_t colour, bool useTextWrapping )
{
    // Text position is pixel coordinate for the top left of the glyph
    uint8_t xCurrentTextPosition = xStartPos;
    uint8_t yCurrentTextPosition = yStartPos;
    uint8_t characterWidth;
    if( fontSize == 8U )
        characterWidth = OLED_FONT8_WIDTH;
    else if( fontSize == 12U )
        characterWidth = OLED_FONT12_WIDTH;
    else if( fontSize == 16U )
        characterWidth = OLED_FONT16_WIDTH;
    else if( fontSize == 20U )
        characterWidth = OLED_FONT20_WIDTH;
    else if( fontSize == 24U )
        characterWidth = OLED_FONT24_WIDTH;
    else
    {
        // Unsupported font size
        return;
    }

    while( *text != 0 )
    {
        if( useTextWrapping == true )
        {
            // Have we gone too far to the right
            if( ( xCurrentTextPosition + characterWidth ) > m_displayWidth )
            {
                xCurrentTextPosition = xStartPos;
                yCurrentTextPosition += fontSize + OLED_WRITE_TEXT_CHARACTER_GAP;
            }
            // Have we've gone too far down
            if( ( yCurrentTextPosition + fontSize ) > m_displayHeight )
            {
                // Ran out of space, stop writing text
                break;
            }
        }

        oled_writeChar( xCurrentTextPosition, yCurrentTextPosition, *text, fontSize, colour );
        ++text;
        xCurrentTextPosition += characterWidth + OLED_WRITE_TEXT_CHARACTER_GAP;
    }
}

int oled_terminalInit( uint8_t fontSize, uint16_t colour )
{
    if( m_terminalBitmapState != e_terminalUninitialised )
        return 2; // Fail as terminal already initialised

    switch( fontSize )
    {
#ifdef OLED_INCLUDE_FONT8
        case 8U:
        {
            m_terminalFontTablePtr = &Font8;
        }
        break;
#endif /* OLED_INCLUDE_FONT8 */
#ifdef OLED_INCLUDE_FONT12
        case 12U:
        {
            m_terminalFontTablePtr = &Font12;
        }
        break;
#endif /* OLED_INCLUDE_FONT12 */
#ifdef OLED_INCLUDE_FONT16
        case 16U:
        {
            m_terminalFontTablePtr = &Font16;
        }
        break;
#endif /* OLED_INCLUDE_FONT16 */
#ifdef OLED_INCLUDE_FONT20
        case 20U:
        {
            m_terminalFontTablePtr = &Font20;
        }
        break;
#endif /* OLED_INCLUDE_FONT20 */
#ifdef OLED_INCLUDE_FONT24
        case 24U:
        {
            m_terminalFontTablePtr = &Font24;
        }
        break;
#endif /* OLED_INCLUDE_FONT24 */
        default:
        {
            // Do nothing, font height not supported
            return 3;
        }
        break;
    }

    m_terminalHeightInLines = m_displayHeight / m_terminalFontSize;

    // Need enough bits to cover the the screen width, might have a few bits unused per row
    // This is so that bytes for each row align, which makes scrolling much much easier
    m_terminalBitmapBytesPerRow = (uint16_t) m_displayWidth / 8U;
    // Integer division rounds down but we need to round up
    if( ( (uint16_t) m_displayWidth % 8U ) != 0U )
        ++m_terminalBitmapBytesPerRow;
    // Now need this many bytes per row
    m_terminalBitmapCallocSize = m_terminalBitmapBytesPerRow * m_displayHeight;

    m_terminalBitmapPtr1 = (uint8_t*) calloc( m_terminalBitmapCallocSize, sizeof( uint8_t ) );
    if( m_terminalBitmapPtr1 == NULL )
        return 1; // Memory allocation failed

    m_terminalBitmapPtr2 = (uint8_t*) calloc( m_terminalBitmapCallocSize, sizeof( uint8_t ) );
    if( m_terminalBitmapPtr1 == NULL )
    {
        // Memory allocation failed
        free( m_terminalBitmapPtr1 );
        m_terminalBitmapPtr1 = NULL;
        return 1;
    }

    m_terminalFontSize = fontSize;
    m_terminalFontColour = colour;
    m_terminalCurrentLine = 0U;
    m_terminalBitmapState = e_terminalBitmap1Next;
    m_terminalIsLineTemp = false;

    return 0; // Success
}

bool oled_terminalIsInit( void )
{
#if defined(OLED_INCLUDE_FONT8) || (defined OLED_INCLUDE_FONT12) || defined(OLED_INCLUDE_FONT16) || defined(OLED_INCLUDE_FONT20) || defined(OLED_INCLUDE_FONT24)
    if( m_terminalBitmapState == e_terminalUninitialised )
        return false;
    else
        return true;
#else
    return false;
#endif
}

int oled_terminalSetHeight( uint8_t newHeightInLines )
{
    if( newHeightInLines == 0U )
        return 1; // Invalid, too few lines
    else if( newHeightInLines > ( m_displayHeight / m_terminalFontSize ) )
        return 1; // Invalid, too many lines for the screen size
    
    m_terminalCurrentLine = ( m_terminalCurrentLine < newHeightInLines ) ? m_terminalCurrentLine : newHeightInLines;

    m_terminalHeightInLines = newHeightInLines;

    return 0;
}

uint8_t oled_terminalGetWidthInCharacters( void )
{
    if( m_terminalBitmapState == e_terminalUninitialised )
        return 0;
    else
        return m_displayWidth / ( m_terminalFontTablePtr->Width + OLED_WRITE_TEXT_CHARACTER_GAP );
}

uint8_t oled_terminalGetHeightInCharacters( void )
{
    if( m_terminalBitmapState == e_terminalUninitialised )
        return 0;
    else
        return m_displayHeight / ( m_terminalFontTablePtr->Width + OLED_WRITE_TEXT_CHARACTER_GAP );
}

void oled_terminalWrite( const char text[] )
{
    m_terminalWrite( text );
    
    // Update module scope variables
    if( m_terminalCurrentLine < m_terminalHeightInLines )
        ++m_terminalCurrentLine;
    m_terminalIsLineTemp = false;
}

void oled_terminalWriteTemp( const char text[] ) 
{
    m_terminalWrite( text );
    
    // Don't update m_terminalCurrentLine for the temp function
    m_terminalIsLineTemp = true;
}

void oled_terminalClear( void )
{
    uint8_t* desiredBitmapPtr = ( m_terminalBitmapState == e_terminalBitmap1Next ) ? m_terminalBitmapPtr1 : m_terminalBitmapPtr2;
    // Set all pixels to off
    for( uint16_t index = 0U; index < m_terminalBitmapCallocSize; index++ )
        desiredBitmapPtr[index] = 0x00U;
    // Push the (now empty) bitmap to the display
    m_terminalPushBitmap();
    // Set the current line back to 0
    m_terminalCurrentLine = 0U;
}

void oled_terminalSetLine( uint8_t line ) 
{
    uint8_t terminalMaxLine = oled_terminalGetHeightInCharacters();
    if( ( line >= 0U ) && ( line <= terminalMaxLine ) )
        m_terminalCurrentLine = line;
    // Otherwise not a valid line number
    
    // If the last line written was temp, you've moved so it won't be getting overwritten
    m_terminalIsLineTemp = false;
}

void oled_terminalSetNewColour( uint16_t colour )
{
    uint8_t* currentBitmapPtr; // What the screen currently has
    uint8_t* desiredBitmapPtr; // We need to change this bitmap to what we want the screen to have next
    if( m_terminalBitmapState == e_terminalBitmap1Next )
    {
        desiredBitmapPtr = m_terminalBitmapPtr1;
        currentBitmapPtr = m_terminalBitmapPtr2;
    }
    else
    {
        desiredBitmapPtr = m_terminalBitmapPtr2;
        currentBitmapPtr = m_terminalBitmapPtr1;
    }

    // Copy current to desired and make current blank to force all pixels to be updated
    for( uint16_t index = 0U; index < m_terminalBitmapCallocSize; index++ )
    {
        desiredBitmapPtr[index] = currentBitmapPtr[index];
        currentBitmapPtr[index] = 0x0;
    }

    // Change the terminal colour
    m_terminalFontColour = colour;

    // Push
    m_terminalPushBitmap();
}

#endif /* defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24 */

void oled_terminalDeinit( void ) 
{
#if defined(OLED_INCLUDE_FONT8) || defined(OLED_INCLUDE_FONT12) || (defined OLED_INCLUDE_FONT16) || defined(OLED_INCLUDE_FONT20) || defined(OLED_INCLUDE_FONT24)
    if( m_terminalBitmapState == e_terminalUninitialised )
        return;

    // Free the memory and set the bitmap pointers to NULL. Never free a NULL pointer
    if( m_terminalBitmapPtr1 != NULL )
    {
        free( m_terminalBitmapPtr1 );
        m_terminalBitmapPtr1 = NULL;
    }
    if( m_terminalBitmapPtr2 != NULL )
    {
        free( m_terminalBitmapPtr2 );
        m_terminalBitmapPtr2 = NULL;
    }
    // Change the bitmap state module scope variable to uninitialised
    m_terminalBitmapState = e_terminalUninitialised;
#endif
}

#ifdef OLED_INCLUDE_SD_IMAGES

int oled_sdWriteImage( const char filename[], uint8_t originX, uint8_t originY )
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    char buf[OLED_SD_BUFFER_SIZE];

    // Mount the SD card
    fr = f_mount( &fs, "0:", 1 );
    if( fr != FR_OK )
        return 1;

    // Open the file that needs to be read
    fr = f_open( &fil, filename, FA_READ );
    if( fr != FR_OK )
        return 2;

    // Read the file and push to the display
    uint8_t x = originX;
    uint8_t y = originY;
    uint8_t imageWidth = 0U;  // Init to invalid number
    uint8_t imageHeight = 0U; // Init to invalid number
    uint16_t nibbleBuffer;
    uint8_t nibblesInBuffer = 0U;
    bool end = false;
    while( f_gets( buf, sizeof(buf), &fil ) )
    {
        for( uint8_t bufferIndex = 0U; bufferIndex < OLED_SD_BUFFER_SIZE; bufferIndex++ )
        {
            if( buf[bufferIndex] == 0 )
                break; // End of buffer
            if( buf[bufferIndex] == 10 )
                continue; // Newline within file
            if( ( buf[bufferIndex] < 32U ) || ( buf[bufferIndex] > 63U ) )
                buf[bufferIndex] = 32; // To avoid bad nibbles over spilling

            // Add the nibble to the nibbleBuffer
            if( nibblesInBuffer == 0U )
            {
                nibbleBuffer = ( (uint8_t) buf[bufferIndex] - 32U ) << 12;
            }
            else if( nibblesInBuffer == 1U )
            {
                nibbleBuffer += ( (uint8_t) buf[bufferIndex] - 32U ) << 8;
            }
            else if( nibblesInBuffer == 2U )
            {
                nibbleBuffer += ( (uint8_t) buf[bufferIndex] - 32U ) << 4;
            }
            else // if( nibbleBuffer == 3U )
            {
                nibbleBuffer += (uint8_t) buf[bufferIndex] - 32U;
            }
            ++nibblesInBuffer;

            // Check if the nibble buffer is full
            if( nibblesInBuffer == 4U )
            {
                // Do something with the full nibbleBuffer
                // Check if the image width and height have been read
                if( imageWidth == 0U )
                {
                    imageWidth = (uint8_t) ( ( nibbleBuffer & 0b1111111100000000U ) >> 8 );
                    imageHeight = (uint8_t) ( nibbleBuffer & 0b0000000011111111U );
                }
                // Otherwise write to the display
                else
                {
                    oled_setPixel( x, y, nibbleBuffer );

                    ++x;
                    if( x == ( originX + imageWidth ) )
                    {
                        ++y;
                        x = originX;
                        if( y == originY + imageHeight )
                        {
                            end = true;
                            break;
                        }
                    }

                }
                // The nibbleBuffer is now considered empty
                nibblesInBuffer = 0U;
            }
        }
        if( end )
            break;
    }

    // Close the file
    fr = f_close( &fil );
    if( fr != FR_OK )
    {
        f_unmount( "0:" );
        return 3;
    }

    // Unmount the SD card
    f_unmount( "0:" );
    
    return 0;
}

#endif // OLED_INCLUDE_SD_IMAGES

#ifdef OLED_INCLUDE_QR_GENERATOR

int oled_printQrCode( const char text[], uint16_t colour1, uint16_t colour2 )
{
    // The buffer size is determined in the qrcodegen.h file
    // This function will use over 8kB of stack memory while in use

    // CONSIDER ADJUSTING THE QR CODE VERSION TO SAVE SOME MEMORY?

    // QR code generation
    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    bool ok = qrcodegen_encodeText( text, tempBuffer, qrcode, errCorLvl,
		qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true );
    if( !ok )
        return 1; // QR code generation failed

    // Display the QR code on the display
    const uint8_t qrSize = (uint8_t) qrcodegen_getSize( qrcode );
    const uint8_t pixelsPerQrBit = m_displayWidth / qrSize;
    const uint8_t qrOrigin = ( m_displayWidth - ( pixelsPerQrBit * qrSize ) ) / 2U;
    uint8_t qrcodeX = 0U;
    uint8_t qrcodeY = 0U;
    uint8_t displayX;
    uint8_t displayY;

    while( qrcodeY < qrSize )
    {
        if( qrcodegen_getModule( qrcode, qrcodeX, qrcodeY ) )
        {
            // Typically you'd set the screen dark here
            for( displayX = qrOrigin + ( qrcodeX * pixelsPerQrBit ); displayX < qrOrigin + ( qrcodeX * pixelsPerQrBit ) + pixelsPerQrBit; displayX++ )
            {
                for( displayY = qrOrigin + ( qrcodeY * pixelsPerQrBit ); displayY < qrOrigin + ( qrcodeY * pixelsPerQrBit ) + pixelsPerQrBit; displayY++ )
                {
                    oled_setPixel( displayX, displayY, colour1 );
                }
            }
        }
        else
        {
            // Typically you'd set the screen bright here
            for( displayX = qrOrigin + ( qrcodeX * pixelsPerQrBit ); displayX < qrOrigin + ( qrcodeX * pixelsPerQrBit ) + pixelsPerQrBit; displayX++ )
            {
                for( displayY = qrOrigin + ( qrcodeY * pixelsPerQrBit ); displayY < qrOrigin + ( qrcodeY * pixelsPerQrBit ) + pixelsPerQrBit; displayY++ )
                {
                    oled_setPixel( displayX, displayY, colour2 );
                }
            }
        }

        ++qrcodeX;
        if( qrcodeX == qrSize )
        {
            qrcodeX = 0U;
            ++qrcodeY;
        }
    }

    // True for dark

    return 0; // Success
}

#endif // defined OLED_INCLUDE_QR_GENERATOR

/* --- MODULE SCOPE FUNCTION IMPLEMENTATIONS ---------------------------------- */

#if defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24
/*
 * Function: m_terminalWriteChar
 * --------------------
 * Write a char to the terminal bitmap
 *
 * bitmapPtr: Pointer to the bitmap to be written to
 * fontTablePtr: Pointer to the relevant font table
 * 
 * returns: void
 */
static inline void m_terminalWriteChar( char character, uint8_t textOriginX, uint8_t textOriginY )
{
    uint8_t* bitmapPtr = ( m_terminalBitmapState == e_terminalBitmap1Next ) ? m_terminalBitmapPtr1 : m_terminalBitmapPtr2;
    // Calculate the width of a character in bytes
    uint8_t fontWidthBytes = ( (uint8_t) m_terminalFontTablePtr->Width / 8U ) + 1U;
    // Find starting position in the array. Note that the first 32 characters of
    // ascii aren't human readable
    uint16_t fontTableIndex = m_terminalFontTablePtr->Height * (uint16_t) fontWidthBytes * (uint16_t) ( character - 32U );
    // fontTableBitPosition records the position within each byte, for the font table
    uint8_t fontTableBitPosition;
    uint16_t bitmapIndex;

    for( uint8_t yPixelPosition = 0U; yPixelPosition < m_terminalFontTablePtr->Height; yPixelPosition++ )
    {
        fontTableBitPosition = 0U;
        for( uint8_t xPixelPosition = 0U; xPixelPosition < m_terminalFontTablePtr->Width; xPixelPosition++ )
        {
            // Check the bit in the font table
            if( ( m_terminalFontTablePtr->table[fontTableIndex] & ( 0b10000000 >> fontTableBitPosition ) ) != 0 )
            {
                // Find the bit within the bitmap that needs to be set high
                bitmapIndex = m_terminalBitmapBytesPerRow * ( textOriginY + yPixelPosition );
                bitmapIndex += ( textOriginX + xPixelPosition ) / 8U;
                // Set the pixel in the bitmap high

                bitmapPtr[bitmapIndex] |= 0b1 << ( ( textOriginX + xPixelPosition ) % 8U);
            }

            ++fontTableBitPosition;
            if( fontTableBitPosition == 8 )
            {
                ++fontTableIndex;
                fontTableBitPosition = 0U;
            }
        }
        ++fontTableIndex;
    }

}

/*
 * Function: m_terminalWrite
 * --------------------
 * Code common to oled_terminalWritea and oled_terminalWriteTemp
 * The changing of m_terminalCurrentLine and m_terminalIsLineTemp is done by the
 * aforementioned functions
 *
 * text: Text to be written
 * 
 * returns: void
 */
static inline void m_terminalWrite( const char text[] )
{
    if( m_terminalBitmapState == e_terminalUninitialised )
        return; // Terminal not ininitialised

    uint8_t* currentBitmapPtr; // What the screen currently has
    uint8_t* desiredBitmapPtr; // We need to change this bitmap to what we want the screen to have next
    if( m_terminalBitmapState == e_terminalBitmap1Next )
    {
        desiredBitmapPtr = m_terminalBitmapPtr1;
        currentBitmapPtr = m_terminalBitmapPtr2;
    }
    else
    {
        desiredBitmapPtr = m_terminalBitmapPtr2;
        currentBitmapPtr = m_terminalBitmapPtr1;
    }

    // If the last line temporary was copy from the previous bitmap except for the temporary line
    if( ( m_terminalIsLineTemp == true ) )
    {
        // Copy everything except the previous line
        uint16_t copyEndIndex;
        if( m_terminalCurrentLine != m_terminalHeightInLines )
        {
            copyEndIndex = (uint16_t) m_terminalCurrentLine * (uint16_t) m_terminalBitmapBytesPerRow * (uint16_t) m_terminalFontSize;
        }
        else
        {
            copyEndIndex = (uint16_t) ( m_terminalCurrentLine - 1 ) * (uint16_t) m_terminalBitmapBytesPerRow * (uint16_t) m_terminalFontSize;
        }

        for( uint16_t bitmapIndex = 0U; bitmapIndex < copyEndIndex; bitmapIndex++ )
        {
            desiredBitmapPtr[bitmapIndex] = currentBitmapPtr[bitmapIndex];
        }
        
        // And set the temporary line which is being overwritten to 0
        uint16_t eraseEndIndex = copyEndIndex + ( m_terminalFontTablePtr->Height * m_terminalBitmapBytesPerRow );
        // Ensure we aren't writing out of array bounds
        eraseEndIndex = ( eraseEndIndex < m_terminalBitmapCallocSize ) ? eraseEndIndex : m_terminalBitmapCallocSize;
        for( uint16_t bitmapIndex = copyEndIndex; bitmapIndex < eraseEndIndex; bitmapIndex++ )
            desiredBitmapPtr[bitmapIndex] = 0x00U;
    }
    // If we've ran out of lines, scroll down when copying to the other bitmap
    else if( m_terminalCurrentLine == m_terminalHeightInLines )
    {
        uint16_t sourceByte = 0U;
        // Shift everything up
        while( ( sourceByte + ( (uint16_t) m_terminalBitmapBytesPerRow * m_terminalFontTablePtr->Height ) ) < m_terminalBitmapCallocSize )
        {
            desiredBitmapPtr[sourceByte] = currentBitmapPtr[sourceByte + ( m_terminalBitmapBytesPerRow * m_terminalFontTablePtr->Height )];
            ++sourceByte;
        }

        // Erase the bottom of the bitmap, so we can put our own text on it
        sourceByte = ( m_terminalHeightInLines - 1 ) * m_terminalFontSize * m_terminalBitmapBytesPerRow;
        while( sourceByte < m_terminalBitmapCallocSize )
        {
            desiredBitmapPtr[sourceByte] = 0x00U; // Background colour
            ++sourceByte;
        }
    }
    // If we have room and don't need to scroll, directly copy one bitmap to the other
    else
    {
        // This copy takes about 50 microseconds for a 128x128
        for( uint16_t index = 0U; index < m_terminalBitmapCallocSize; index++ )
        {
            desiredBitmapPtr[index] = currentBitmapPtr[index];
        }
        // And then erase the line we want to write on, in case the terminal set line function has been used
        uint16_t index;
        // Set the index starting point
        if( m_terminalCurrentLine != m_terminalHeightInLines )
        {
            index = (uint16_t) m_terminalCurrentLine * (uint16_t) m_terminalBitmapBytesPerRow * (uint16_t) m_terminalFontSize;
        }
        else
        {
            index = (uint16_t) ( m_terminalCurrentLine - 1 ) * (uint16_t) m_terminalBitmapBytesPerRow * (uint16_t) m_terminalFontSize;
        }
        // Determine erase end point
        uint16_t eraseEndIndex = index + ( m_terminalFontTablePtr->Height * m_terminalBitmapBytesPerRow);
        // Ensure we won't be writing out of bounds
        eraseEndIndex = ( eraseEndIndex < m_terminalBitmapCallocSize ) ? eraseEndIndex : m_terminalBitmapCallocSize;
        while( index < eraseEndIndex )
        {
            desiredBitmapPtr[index] = 0x00U;
            ++index;
        }
    }
    
    // --- Add the text to the bitmap ---
    // These positions are of the glyph origins
    uint8_t xCurrentTextPosition = 0U;
    uint8_t yCurrentTextPosition;
    if( m_terminalCurrentLine == m_terminalHeightInLines )
    {
        // If the display just scrolled, we need to write on the previous line else we'll be writing off the display
        yCurrentTextPosition = ( m_terminalCurrentLine - 1 ) * m_terminalFontSize;
    }
    else
        yCurrentTextPosition = m_terminalCurrentLine * m_terminalFontSize;
    
    uint8_t characterWidth = m_terminalFontTablePtr->Width;
    
    // Put the characters on the bitmap
    while( *text != 0 )
    {
        // Check if we've gone too far to the right
        if( ( xCurrentTextPosition + characterWidth ) > m_displayWidth )
        {
            // No text wrapping implemented for the terminal so just stop
            break;
        }
        
        // Write the character
        m_terminalWriteChar( *text, xCurrentTextPosition, yCurrentTextPosition );

        ++text;
        xCurrentTextPosition += characterWidth + OLED_WRITE_TEXT_CHARACTER_GAP;
    }

    // Push the bitmap
    m_terminalPushBitmap();

}

/*
 * Function: m_terminalPushBitmap
 * --------------------
 * Push the current terminal bitmap to the screen. This function does
 * change the value of m_terminalBitmapState
 *
 * parameters: none
 *
 * returns: void
 */
static void m_terminalPushBitmap( void )
{
    // Position within each byte in the bitmap array
    uint8_t bitPosition = 0U;
    uint16_t bitmapIndex = 0U;
    // Position on the display
    uint8_t displayPositionX = 0U;
    uint8_t displayPositionY = 0U;
    // Check which bitmap to push
    uint8_t* desiredStateBitmap;
    uint8_t* currentStateBitmap;
    if( m_terminalBitmapState == e_terminalBitmap1Next )
    {
        desiredStateBitmap = m_terminalBitmapPtr1;
        currentStateBitmap = m_terminalBitmapPtr2;
    }
    else
    {
        desiredStateBitmap = m_terminalBitmapPtr2;
        currentStateBitmap = m_terminalBitmapPtr1;
    }

    while( displayPositionY != m_displayHeight )
    {
        // Check if a pixel needs to be turned on
        if( ( ( currentStateBitmap[bitmapIndex] & ( 1 << bitPosition ) ) == 0 ) &&
            ( ( desiredStateBitmap[bitmapIndex] & ( 1 << bitPosition ) ) != 0 ) )
        {
            oled_setPixel( displayPositionX, displayPositionY, m_terminalFontColour );
        }
        // Check if a pixel needs to be turned off
        else if( ( ( currentStateBitmap[bitmapIndex] & ( 1 << bitPosition ) ) != 0 ) &&
            ( ( desiredStateBitmap[bitmapIndex] & ( 1 << bitPosition ) ) == 0 ) )
        {
            oled_setPixel( displayPositionX, displayPositionY, 0x0000 );
        }
        // Otherwise leave the screen as it is
        
        ++bitPosition;
        if( bitPosition == 8U )
        {
            bitPosition = 0U;
            ++bitmapIndex;
        }
        ++displayPositionX;
        if( displayPositionX == m_displayWidth )
        {
            displayPositionX = 0U;
            ++displayPositionY;
        }
    }

    // Change the next bitmap value
    if( m_terminalBitmapState == e_terminalBitmap1Next )
        m_terminalBitmapState = e_terminalBitmap2Next;
    else
        m_terminalBitmapState = e_terminalBitmap1Next;
}

#endif // defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24

#ifdef OLED_INCLUDE_LOADING_CIRCLE

/*
 * Function: m_loadingCircleProcessQuadrant
 * --------------------
 * This is used for drawing each quadrant on the bitmap using the m_loadingCircleSetBitmap
 * function
 *
 * bitmapPtr: Pointer to the loading circle bitmap that needs to be changed
 * xLowerBound: Left most value of x for the quadrant
 * xUpperBound: Right most value of x for the quadrant
 * yIsPositive: Is this one of the two quadrants which is higher on the display?
 *              Note: If yIsPositive is true, then lower y value will be used, because
 *              the positive y axis extends down the display
 * angle: Amount of quadrant which is completed, clockwise in degrees
 *
 * returns: void
 */
static inline void m_loadingCircleProcessQuadrant( uint8_t* bitmapPtr, uint8_t xLowerBound,
    uint8_t xUpperBound, bool yIsPositive, uint8_t angle )
{
    uint8_t limit; // May be upper or lower bound for y
    uint8_t triangleWidth; // Referring to pythag triangle

    // Delete everything in this quarter of the bitmap
    if( yIsPositive )
    {
        for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
        {
            for( uint8_t y = 0U; y < ( m_loadingCircleOuterRadius - 1U ); y++ )
            {
                m_loadingCircleSetBitmap( bitmapPtr, x, y, false );
            }
        }
    }
    else
    {
        for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
        {
            for( uint8_t y = ( m_loadingCircleOuterRadius - 1U ); y < ( ( m_loadingCircleOuterRadius * 2U ) - 1U ); y++ )
            {
                m_loadingCircleSetBitmap( bitmapPtr, x, y, false );
            }
        }
    }

    // Now add the circle quadrants in as needed
    if( angle >= 90U ) // Then angle is 90U
    {
        if( yIsPositive )
        {
            if( xLowerBound == 0 )
                triangleWidth = m_loadingCircleOuterRadius;
            else
                triangleWidth = 0U;
            
            for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
            {
                limit = ( m_loadingCircleOuterRadius - 1U ) - (uint8_t) ( sqrt( ( (uint16_t) m_loadingCircleOuterRadius * (uint16_t) m_loadingCircleOuterRadius ) - ( (uint16_t) triangleWidth * (uint16_t) triangleWidth ) ) );
                for( uint8_t y = limit; y < ( m_loadingCircleOuterRadius - 1U ); y++ )
                {
                    m_loadingCircleSetBitmap( bitmapPtr, x, y, true );
                }
                if( xLowerBound == 0 )
                    --triangleWidth;
                else
                    ++triangleWidth;
                
            }
        }
        else
        {
            if( xLowerBound == 0 )
                triangleWidth = m_loadingCircleOuterRadius;
            else
                triangleWidth = 0U;
            
            for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
            {
                limit = ( m_loadingCircleOuterRadius - 1U ) + (uint8_t) ( sqrt( ( m_loadingCircleOuterRadius * m_loadingCircleOuterRadius ) - ( triangleWidth * triangleWidth ) ) );
                for( uint8_t y = ( m_loadingCircleOuterRadius - 1U ); y < limit; y++ )
                {
                    m_loadingCircleSetBitmap( bitmapPtr, x, y, true );
                }
                if( xLowerBound == 0 )
                    --triangleWidth;
                else
                    ++triangleWidth;
                
            }
        }
    }
    else if( angle > 0U )
    {
        // Need to draw a partial quadrant
        int32_t lineValue;      // y in y=mx+c, using bitmap coordinates
        int32_t circleValue;    // vertical distance between centre of circle and circumference for a given x
        uint8_t triangleWidth;
        uint8_t limit; // For quadrants where we need to draw up to the minimum of two values
        const int32_t gradientScaleFactor = 100;

        if( yIsPositive )
        {
            if( xLowerBound == 0U ) // Quadrant 3
            {
                // To make the transition between Q2 and Q3 smoother
                if( angle < 4U )
                {
                    angle += 2;
                }

                const int32_t cosTheta = intcos( (int16_t) angle );
                if( cosTheta == 0U )
                {
                    // No 0 division errors
                    return;
                }

                const int32_t gradientScaled = ( intsin( (int16_t) angle ) * gradientScaleFactor ) / cosTheta; // scale up the gradient, so that we can use integers
                const int32_t c = ( (int32_t) m_loadingCircleOuterRadius - 1 ) - ( ( gradientScaled * ( (int32_t) m_loadingCircleOuterRadius - 1 ) ) / gradientScaleFactor );

                triangleWidth = m_loadingCircleOuterRadius;
                for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
                {
                    lineValue = ( ( gradientScaled * x ) / gradientScaleFactor ) + c;
                    circleValue = ( (int32_t) m_loadingCircleOuterRadius - 1 ) - (int32_t) ( sqrt( ( (uint16_t) m_loadingCircleOuterRadius * (uint16_t) m_loadingCircleOuterRadius ) - ( (uint16_t) triangleWidth * (uint16_t) triangleWidth ) ) );

                    if( circleValue < 0 )
                        circleValue = 0;
                    else if( circleValue > 0xFF )
                        circleValue = 0xFF;
                    
                    if( lineValue < 0 )
                        lineValue = 0;
                    else if( lineValue > 0xFF )
                        lineValue = 0xFF;

                    limit = ( lineValue > circleValue ) ? (uint8_t) lineValue : (uint8_t) circleValue;

                    for( uint8_t y = limit; y < (m_loadingCircleOuterRadius - 1U); y++ )
                    {
                        m_loadingCircleSetBitmap( bitmapPtr, x, y, true );
                    }
                    --triangleWidth;
                }
            }
            else // Quadrant 0
            {
                // Angle for the y=mx+c line is 90-angle
                const int32_t cosTheta = intcos( (int16_t) ( 90U - angle ) );
                if( cosTheta == 0 )
                {
                    // No div0 errors please
                    return;
                }

                const int32_t gradientScaled = -1 * ( intsin( (int16_t) ( 90U - angle ) ) * gradientScaleFactor ) / cosTheta; // scale up the gradient, so that we can use integers
                const int32_t c = ( (int32_t) m_loadingCircleOuterRadius - 1 ) - ( ( gradientScaled * ( (int32_t) m_loadingCircleOuterRadius - 1 ) ) / gradientScaleFactor );

                triangleWidth = 0U;
                for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
                {
                    lineValue = ( ( gradientScaled * x ) / gradientScaleFactor ) + c;
                    circleValue = ( (int32_t) m_loadingCircleOuterRadius - 1 ) - (int32_t) ( sqrt( ( (uint16_t) m_loadingCircleOuterRadius * (uint16_t) m_loadingCircleOuterRadius ) - ( (uint16_t) triangleWidth * (uint16_t) triangleWidth ) ) );

                    if( circleValue >= lineValue ) // SHOULD THIS BE A > NOT >= ?
                        break; // Intersection

                    if( ( lineValue < 0 ) || ( lineValue > ( ( (int32_t) m_loadingCircleOuterRadius * 2 ) - 2 ) ) ||
                        ( circleValue < 0 ) || ( circleValue > ( ( (int32_t) m_loadingCircleOuterRadius * 2 ) - 2 ) ) )
                    {
                        ++triangleWidth;
                        continue;
                    }
                    
                    for( uint8_t y = (uint8_t) circleValue; y < (uint8_t) lineValue; y++ )
                    {
                        m_loadingCircleSetBitmap( bitmapPtr, x, y, true );
                    }
                    ++triangleWidth;
                }
            }
        }
        else // Then yIsPositive == false
        {
            if( xLowerBound == 0U ) // Quadrant 2
            {
                // To make the transition between Q2 and Q3 smoother
                if( angle > ( 90U - 4U ) )
                {
                    angle -= 2U;
                }

                // Angle for the y=mx+c line is 90-angle
                const int32_t cosTheta = intcos( (int16_t) ( 90U - angle ) );
                if( cosTheta == 0 )
                {
                    // No 0 division errors
                    return;
                }

                const int32_t gradientScaled = ( intsin( (int16_t) ( 90U - angle ) ) * gradientScaleFactor * -1 ) / cosTheta; // scale up the gradient, so that we can use integers
                const int32_t c = ( (int32_t) m_loadingCircleOuterRadius - 1 ) - ( ( gradientScaled * ( (int32_t) m_loadingCircleOuterRadius - 1 ) ) / gradientScaleFactor );

                triangleWidth = m_loadingCircleOuterRadius;
                for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
                {
                    lineValue = ( ( gradientScaled * x ) / gradientScaleFactor ) + c;
                    circleValue = ( (int32_t) m_loadingCircleOuterRadius - 1 ) + (int32_t) ( sqrt( ( (uint16_t) m_loadingCircleOuterRadius * (uint16_t) m_loadingCircleOuterRadius ) - ( (uint16_t) triangleWidth * (uint16_t) triangleWidth ) ) );

                    if( ( lineValue >= circleValue ) ||  // SHOULD THIS BE A > NOT >= ?
                        ( lineValue < 0 ) || ( lineValue > ( ( (int32_t) m_loadingCircleOuterRadius * 2 ) - 2 ) ) ||
                        ( circleValue < 0 ) || ( circleValue > ( ( (int32_t) m_loadingCircleOuterRadius * 2 ) - 2 ) ) )
                    {
                        --triangleWidth;
                        continue;
                    }
                    
                    for( uint8_t y = (uint8_t) lineValue; y < (uint8_t) circleValue; y++ )
                    {
                        m_loadingCircleSetBitmap( bitmapPtr, x, y, true );
                    }
                    --triangleWidth;
                }
            }
            else // Quadrant 1
            {   
                const int32_t cosTheta = intcos( (int16_t) angle );
                if( cosTheta == 0 )
                {
                    // No 0 division errors
                    return;
                }

                const int32_t gradientScaled = ( intsin( (int16_t) angle ) * gradientScaleFactor ) / cosTheta; // scale up the gradient, so that we can use integers
                const int32_t c = ( (int32_t) m_loadingCircleOuterRadius - 1 ) - ( ( gradientScaled * ( (int32_t) m_loadingCircleOuterRadius - 1 ) ) / gradientScaleFactor );

                triangleWidth = 0U;
                for( uint8_t x = xLowerBound; x < xUpperBound; x++ )
                {
                    lineValue = ( ( gradientScaled * x ) / gradientScaleFactor ) + c;
                    circleValue = ( (int32_t) m_loadingCircleOuterRadius - 1 ) + (int32_t) ( sqrt( ( (uint16_t) m_loadingCircleOuterRadius * (uint16_t) m_loadingCircleOuterRadius ) - ( (uint16_t) triangleWidth * (uint16_t) triangleWidth ) ) );

                    if( circleValue < 0 )
                        circleValue = 0;
                    else if( circleValue > 0xFF )
                        circleValue = 0xFF;
                    
                    if( lineValue < 0 )
                        lineValue = 0;
                    else if( lineValue > 0xFF )
                        lineValue = 0xFF;

                    limit = ( lineValue < circleValue ) ? (uint8_t) lineValue : (uint8_t) circleValue;
                    
                    for( uint8_t y = ( m_loadingCircleOuterRadius - 1U ); y < (uint8_t) limit; y++ )
                    {
                        m_loadingCircleSetBitmap( bitmapPtr, x, y, true );
                    }
                    ++triangleWidth;
                }
            }
        }
    }
}

/*
 * Function: m_loadingCircleSetBitmap
 * --------------------
 * Used to set values in the loading circle bitmap. The top left of the bitmap is 0,0,
 * and this may not be 0,0 on the display.
 *
 * bitmapPtr: Pointer to the loading circle bitmap that needs to be changed
 * x, y: Coordinates of the position within the the bitmap to be changed
 * val: Set the bit within the bitmap to high or low
 *
 * returns: void
 */
static inline void m_loadingCircleSetBitmap( uint8_t* bitmapPtr, uint8_t x, uint8_t y, bool val ) 
{
    if( m_loadingBarState != e_loadingBarStateCircle )
        return;

    // The function that calls this function should ensure that the bitmaps are properly initialised
    uint16_t bitNumber = ( m_loadingCircleBitmapWidth * y ) + x;
    uint16_t byteNumber = bitNumber / 8U;
    // Check it's within bounds
    if( byteNumber < m_loadingBarCallocSize )
    {
        if( val )
            bitmapPtr[byteNumber] |= ( 0b10000000 >> ( bitNumber % 8U ) ); // Set a bit high
        else
            bitmapPtr[byteNumber] &= ~( 0b10000000 >> ( bitNumber % 8U ) ); // Set a bit low
    }
    else // TEMP
    {
        printf("Prevented writing outside of loading circle bitmap!\n");
    }
}

#endif // OLED_INCLUDE_LOADING_CIRCLE

/*
 * Function: m_displayInit
 * --------------------
 * Initialise the display by writing data to registers
 *
 * parameters: none
 *
 * returns: void
 */
static inline void m_displayInit( void )
{
    // CS should be set low (active) prior to calling this function
    // Now for some inexplicable magic from the manufacturer. Comments here are from the manufacturer
    m_writeReg(0xFD);  // Command lock
    m_writeData(0x12);
    m_writeReg(0xFD);  // Command lock
    m_writeData(0xB1);

    m_writeReg(0xAE);  // Display off
    m_writeReg(0xA4);  // Normal Display mode

    m_writeReg(0x15);  // Set column address
    m_writeData(0x00); // Column address start 00
    m_writeData(0x7F); // Column address end 127
    m_writeReg(0x75);  // Set row address
    m_writeData(0x00); // Row address start 00
    m_writeData(0x7F); // Row address end 127    

    m_writeReg(0xB3);
    m_writeData(0xF1);

    m_writeReg(0xCA);  
    m_writeData(0x7F);

    m_writeReg(0xA0);  // Set re-map & data format
    m_writeData(0x74); // Horizontal address increment

    m_writeReg(0xA1);  // Set display start line
    m_writeData(0x00); // Start 00 line

    m_writeReg(0xA2);  // Set display offset
    m_writeData(0x00);

    m_writeReg(0xAB);  
    m_writeReg(0x01);  

    m_writeReg(0xB4);  
    m_writeData(0xA0);   
    m_writeData(0xB5);  
    m_writeData(0x55);    

    m_writeReg(0xC1);  
    m_writeData(0xC8); 
    m_writeData(0x80);
    m_writeData(0xC0);

    m_writeReg(0xC7);  
    m_writeData(0x0F);

    m_writeReg(0xB1);  
    m_writeData(0x32);

    m_writeReg(0xB2);  
    m_writeData(0xA4);
    m_writeData(0x00);
    m_writeData(0x00);

    m_writeReg(0xBB);  
    m_writeData(0x17);

    m_writeReg(0xB6);
    m_writeData(0x01);

    m_writeReg(0xBE);
    m_writeData(0x05);

    m_writeReg(0xA6);
}

/*
 * Function: m_chipSelect
 * --------------------
 * Select the display as an SPI device by setting CS to low
 *
 * parameters: none
 *
 * returns: void                                                                         
 */
static inline void m_chipSelect( void )
{
    gpio_put( PICO_DEFAULT_SPI_CSN_PIN, 0 );
}

/*
 * Function: m_chipDeselect
 * --------------------
 * Deselct the display as an SPI device by setting CS to high
 *
 * parameters: none
 *
 * returns: void
 */
static inline void m_chipDeselect( void )
{
    gpio_put( PICO_DEFAULT_SPI_CSN_PIN, 1 );
}

/*
 * Function: m_writeReg
 * --------------------
 * Select register to write data to
 *
 * parameters: none
 *
 * returns: void
 */
static inline void m_writeReg( uint8_t reg )
{
    gpio_put( m_dcPin, 0 );
    if( m_spiInstance == 0 )
        spi_write_blocking( spi0, &reg, 1 );
    else
        spi_write_blocking( spi1, &reg, 1 );
}

/*
 * Function: m_writeData
 * --------------------
 * Write data to the selected register
 *
 * parameters: none
 *
 * returns: void
 */
static inline void m_writeData( uint8_t data )
{
    gpio_put( m_dcPin, 1 );
    if( m_spiInstance == 0 )
        spi_write_blocking( spi0, &data, 1 );
    else
        spi_write_blocking( spi1, &data, 1 );
}
