#ifndef OLED_HPP
#define OLED_HPP

/* Settings for this module ------------------------------------------------ */
#define OLED_INCLUDE_TEST_FUNCTION
#define OLED_INCLUDE_LOADING_BAR_HORIZONTAL
#define OLED_INCLUDE_LOADING_CIRCLE
#define OLED_INCLUDE_FONT8                      // Uses ~760 bytes
#define OLED_INCLUDE_FONT12                     // Uses ~1144 bytes
#define OLED_INCLUDE_FONT16                     // Uses ~3044 bytes
#define OLED_INCLUDE_FONT20                     // Uses ~3804 bytes
#define OLED_INCLUDE_FONT24                     // Uses ~6844 bytes
#define OLED_WRITE_TEXT_CHARACTER_GAP     ( 0 ) // Number of pixels between characters
#define OLED_INCLUDE_SD_IMAGES
#define OLED_INCLUDE_QR_GENERATOR

#include <stdint.h>

#ifdef OLED_INCLUDE_SD_IMAGES
#include "sd_card.h"
#include "ff.h"
#endif

/*
 * Function: oled_init
 * --------------------
 * Initialises GPIO and SPI to use the OLED display
 *
 * dinPin: GPIO on pico which is connected to din on display. Will be used for SPI
 * clkPin: GPIO on pico connected to clk (clock) on the display. Will be used for SPI
 * csPin: GPIO on pico connected to CS (Chip Select) on the display. Will be used as GPIO
 * dcPin: GPIO on pico connected to DC (Data/Command) on the display. Will be used as GPIO
 * rstPin: GPIO on pico connected to RST (Reset) on the display, Will be used as GPIO
 * spiOutput: set to 0 for pico SPI0 and 1 for pico SPI1. See pico pin diagram, depends on your din and clk
 *
 * returns: void
 */
int oled_init( int8_t dinPin, int8_t clkPin, int8_t csPin, int8_t dcPin,
    int8_t rstPin, int8_t spiOutput, unsigned int baudrate, uint8_t displayWidth,
    uint8_t displayHeight );


/*
 * Function: oled_clear
 * --------------------
 * Set the display to black
 *
 * parameters: none
 *
 * returns: void
 */
void oled_clear( void );

/*
 * Function: oled_deinitAll
 * --------------------
 * Deinitialises any terminal or loading bar/circle which currently
 * has a heap allocation
 *
 * parameters: none
 *
 * returns: void
 */
void oled_deinitAll( void );

/*
 * Function: oled_setPixel
 * --------------------
 * Set the colour of a specific pixel
 *
 * x: x coordinate of pixel
 * y: y coordinate of pixel
 * colour: Pixel colour in RGB565 format
 *
 * returns: void
 */
void oled_setPixel( uint8_t x, uint8_t y, uint16_t colour );

/*
 * Function: oled_fill
 * --------------------
 * Set the colour of a specific pixel
 *
 * x1: x coordinate of corner 1
 * y1: y coordinate of corner 1
 * x2: x coordinate of corner 2
 * y2: y coordinate of corner 2
 * colour: Pixel colour in RGB565 format
 *
 * returns: void
 */
void oled_fill( uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t colour );

/*
 * Function: oled_drawLineBetweenPoints
 * --------------------
 * Draw a line between two cartesian points using Bresenham's line algorithm
 *
 * x1, y1, x2, y2: Coordinates for line
 * colour: Colour of line in RGB565
 * thickness: 0 for thin line, 2 is fairly thick
 *
 * returns: void
 */
void oled_drawLineBetweenPoints( uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, 
    uint16_t colour, uint8_t thickness );

#ifdef OLED_INCLUDE_TEST_FUNCTION
/*
 * Function: oled_test
 * --------------------
 * Removes everything on the screen, setting every pixel to black
 *
 * parameters: none
 *
 * returns: void
 */
void oled_test( void );
#endif /* OLED_INCLUDE_TEST_FUNCTION */

#ifdef OLED_INCLUDE_LOADING_BAR_HORIZONTAL
/*
 * Function: oled_loadingBarInit
 * --------------------
 * Initialise a loading bar, calls calloc to create bitmaps
 *
 * x1, y1: coordinates of corner 1
 * x2, y2: coordinates of corner opposite corner 1
 * colour: Pixel colour in RGB565 format
 * 
 * returns: int 0 on success
 *          1 on fail due to loading bar already being initialised
 *          2 on fail due to failed calloc
 */
int oled_loadingBarInit( uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, 
    uint16_t colour );

/*
 * Function: oled_loadingBarIsInit
 * --------------------
 * Returns true if the loading bar is initialised
 *
 * parameters: none
 *
 * returns: true if loading bar is initialised
 */
bool oled_loadingBarIsInit( void );

/*
 * Function: oled_loadingBarDisplay
 * --------------------
 * Set the colour of a specific pixel
 *
 * progress: 0 = 0% complete, 255 = 100% complete
 *
 * returns: void
 */
void oled_loadingBarDisplay( uint8_t progress );

#endif /* OLED_INCLUDE_LOADING_BAR_HORIZONTAL */

/*
 * Function: oled_loadingBarDeinit
 * --------------------
 * Frees the loading bar bitmaps, allows you to 
 *
 * parameters: none
 *
 * returns: void
 */
void oled_loadingBarDeinit();

#ifdef OLED_INCLUDE_LOADING_CIRCLE
/*
 * Function: oled_loadingCircleInit
 * --------------------
 * Initialise the loading circle, calloc some bitmaps
 *
 * originX, originY: Coordinates for the centre of the circle
 * outerRadius: Radius of the loading circle
 * innerRadius: Make it a loading donut with an inner radius of this size
 * colour: colour of the loading circle in RGB565
 *
 * returns: int 0 on success
 *          1 on fail due to loading bar already being initialised
 *          2 on fail due to failed calloc
 *          3 on fail due to bad parameters
 */
int oled_loadingCircleInit( uint8_t originX, uint8_t originY, uint8_t outerRadius, 
    uint8_t innerRadius, uint16_t colour );

/*
 * Function: oled_loadingCircleIsInit
 * --------------------
 * Returns true if the loading circle is initialised
 *
 * parameters: none
 *
 * returns: true if the loading circle is initialised
 */
bool oled_loadingCircleIsInit( void );

/*
 * Function: oled_loadingCircleDisplay
 * --------------------
 * Update the display with the loading circle
 *
 * progress: 0 to 252 (NOT 255), 0 is not started and 252 is finished
 *
 * returns: void
 */
void oled_loadingCircleDisplay( uint8_t progress );

#endif /* OLED_INCLUDE_LOADING_CIRCLE */

/*
 * Function: oled_loadingCircleDeinit
 * --------------------
 * Deinitialised the loading circle/donut, and free the allocated bitmap memory
 *
 * parameters: none
 *
 * returns: void
 */
void oled_loadingCircleDeinit( void );

#if defined(OLED_INCLUDE_FONT8) || (defined OLED_INCLUDE_FONT12) || defined(OLED_INCLUDE_FONT16) || defined(OLED_INCLUDE_FONT20) || defined(OLED_INCLUDE_FONT24)
#define OLED_FONT8_WIDTH                ( 5U )
#define OLED_FONT12_WIDTH               ( 7U )
#define OLED_FONT16_WIDTH               ( 11U )
#define OLED_FONT20_WIDTH               ( 14U )
#define OLED_FONT24_WIDTH               ( 17U )

/*
 * Function: oled_writeChar
 * --------------------
 * Print a character to the display
 *
 * x: x coordinate of the top left position for the glyph
 * y: y coordinate of the top left position for the glyph
 * asciiCharacter: Character to be printed at specified position
 * fontSize: Height of character in pixels, can be 8, 12, 16, 20 or 24
 * colour: Character colour in RGB565 format
 *
 * returns: void
 */
void oled_writeChar( uint8_t x, uint8_t y, char asciiCharacter, uint8_t fontSize, uint16_t colour );

/*
 * Function: oled_writeText
 * --------------------
 * Print a character array to the display.
 *
 * xStartPos: Display x coordinate of the top left of the first character to be written
 * yStartPos: Display y coordinate of the top left of the first character to be written
 * text: Text to write to display
 * fontSize: Height of text in pixels, can be 8, 12, 16, 20 or 24
 * colour: Text colour in RGB565 format
 * useTextWrapping: If true, when the end of the display is reached a new line will be started
 *
 * returns: void
 */
void oled_writeText( uint8_t xStartPos, uint8_t yStartPos, const char text[],
    uint8_t fontSize, uint16_t colour, bool useTextWrapping );

/*
 * Function: oled_terminalInit
 * --------------------
 * Initialise a terminal for the display. This will create a bitmap array which
 * will use ceil( ( displayWidth * displayHeight * 2 ) / 8 ) bytes of memory
 * This is 4096 bytes for a 128x128 display
 *
 * fontSize: Height of text in pixels, can be 8, 12, 16, 20 or 24
 * colour: Colour of text in RGB 565
 *
 * returns: int 0 on success
 *          1 on failed malloc
 *          2 if terminal is already initialised
 *          3 if font size is not valid
 */
int oled_terminalInit( uint8_t fontSize, uint16_t colour );

/*
 * Function: oled_terminalIsInit
 * --------------------
 * Returns true if the terminal is initialised
 *
 * parameters: none
 *
 * returns: true if the terminal is initialised
 */
bool oled_terminalIsInit( void );

/*
 * Function: oled_terminalSetHeight
 * --------------------
 * Set the max height of the terminal in lines. By default, the terminal init
 * function will make the terminal as tall as possible.
 *
 * newHeightInLines: Maximum height of the terminal in lines
 *
 * returns: true if successful
 */
int oled_terminalSetHeight( uint8_t newHeightInLines );

/*
 * Function: oled_terminalGetWidthInCharacters
 * --------------------
 * Return the width of the terminal in characters
 *
 * parameters: none
 *
 * returns: uint8_t width of terminal in characters
 */
uint8_t oled_terminalGetWidthInCharacters();

/*
 * Function: oled_terminalGetHeightInCharacters
 * --------------------
 * Return the height of the terminal in lines
 *
 * parameters: none
 *
 * returns: uint8_t height of terminal in lines
 */
uint8_t oled_terminalGetHeightInCharacters();

/*
 * Function: oled_terminalWrite
 * --------------------
 * Write a line to the terminal
 * If there isn't enough vertical space on the screen, the existing text will
 * scroll up to make room for the new line
 *
 * text: Text to be written to the display
 *
 * returns: void
 */
void oled_terminalWrite( const char text[] );

/*
 * Function: oled_terminalWrite
 * --------------------
 * Write a temporary line to the terminal, next time a line is written it will
 * replace this temporary line
 *
 * text: Text to be written to the display
 *
 * returns: void
 */
void oled_terminalWriteTemp( const char text[] );

/*
 * Function: oled_terminalClear
 * --------------------
 * Clear the display, next time a line is written to the terminal, it will
 * appear at the top of the screen
 *
 * parameters: void
 *
 * returns: void
 */
void oled_terminalClear( void );

/*
 * Function: oled_terminalSetLine
 * --------------------
 * Set line to be written on. All lines below will be deleted
 *
 * line: Terminal line to write on. Top of terminal is line 0!
 *
 * returns: void
 */
void oled_terminalSetLine( uint8_t line );

/*
 * Function: oled_terminalSetNewColour
 * --------------------
 * Change the colour of the terminal
 *
 * colour: Colour to change the terminal to
 *
 * returns: void
 */
void oled_terminalSetNewColour( uint16_t colour );

#endif /* defined OLED_INCLUDE_FONT8 || defined OLED_INCLUDE_FONT12 || defined OLED_INCLUDE_FONT16 || defined OLED_INCLUDE_FONT20 || defined OLED_INCLUDE_FONT24 */

/*
 * Function: oled_terminalClear
 * --------------------
 * Write a temporary line to the terminal, next time a line is written it will
 * replace this temporary line
 *
 * parameters: void
 *
 * returns: void
 */
void oled_terminalDeinit( void );

#ifdef OLED_INCLUDE_SD_IMAGES

/*
 * Function: oled_sdWriteImage
 * --------------------
 * Write an image to the screen has been encoded into a txt file. This is probably
 * very inefficient, but it does what it needs to do for me, for now. The txt files
 * are created using a .py file. You can specify where the image is drawn on the 
 * screen by using the xOrigin and yOrigin.
 * IMPORTANT: When this function is called, sd_init_driver() must have already been
 * called (and returned 0), and the sd card should be unmounted.
 *
 * filename: Name of encoded txt file, e.g. "image1.txt"
 *
 * returns: int 0 on success
 *              1 on fail due to failed SD card mounting
 *              2 on fail because the specified file couldn't be opened
 *              3 on fail because the file couldn't be closed
 */
int oled_sdWriteImage( const char filename[], uint8_t originX, uint8_t originY );

#endif // defined OLED_INCLUDE_SD_IMAGES

#ifdef OLED_INCLUDE_QR_GENERATOR

/*
 * Function: oled_printQrCode
 * --------------------
 * Draw a QR code on the screen
 *
 * text: The text to be encoded to the display
 * colour1: This is typically black (0x0000U)
 * colour2: This is typically white (0xFFFFU)
 *
 * returns: int 0 on success
 *              1 on fail due to QR generation library failing
 */
int oled_printQrCode( const char text[], uint16_t colour1, uint16_t colour2 );

#endif // defined OLED_INCLUDE_QR_GENERATOR

#endif /* OLED_HPP */
