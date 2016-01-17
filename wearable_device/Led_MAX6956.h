/*
Dyno V5.0 rev A
Written by: Samuel Huberman, Stefan Kalchmair
Date updated: 6-June-2015
LED driver MAX6956
*/

#ifndef _LED_MAX6956_H
#define _LED_MAX6956_H

#include <Wire.h>

#define NUMBER_OF_LEDS 2
#define NUMBER_OF_LED_PATTERNS            3       // Number of LED illumination patterns: e.g. all 680 nm LEDs, one 810 nm LED and dark measurement

#define LED1_NAME  "650nm"
#define LED2_NAME  "855nm"

#define RED_LED     0
#define GREEN_LED   1
#define BLUE_LED    2
#define RED_LED_NAME  "red"
#define GREEN_LED_NAME  "green"
#define BLUE_LED_NAME  "blue"

#define BUTTON1_PORT       0x3F
#define BUTTON2_PORT       0x3E
#define BATTERY_STAT_PORT  0x3D

typedef enum {
  LED1  = 0,
  LED2  = 1,
  LAST_LED = LED2,
} Portnum_t;

#define P12_REG   0x2C // Port 12
#define P13_REG   0x2D // Port 13
#define P14_REG   0x2E // Port 14
#define P15_REG   0x2F // Port 15
#define P16_REG   0x30 // Port 16
#define P17_REG   0x31 // Port 17
#define P18_REG   0x32 // Port 18
#define P19_REG   0x33 // Port 19
#define P20_REG   0x34 // Port 20
#define P21_REG   0x35 // Port 21
#define P22_REG   0x36 // Port 22
#define P23_REG   0x37 // Port 23
#define P24_REG   0x38 // Port 24
#define P25_REG   0x39 // Port 25
#define P26_REG   0x3A // Port 26
#define P27_REG   0x3B // Port 27
#define P28_REG   0x3C // Port 28
#define P29_REG   0x3D // Port 29
#define P30_REG   0x3E // Port 30

// current control registers
#define P12_CURR_REG   0x16 // Port 12
#define P13_CURR_REG   0x16 // Port 13
#define P14_CURR_REG   0x17 // Port 14
#define P15_CURR_REG   0x17 // Port 15
#define P16_CURR_REG   0x18 // Port 16
#define P17_CURR_REG   0x18 // Port 17
#define P18_CURR_REG   0x19 // Port 18
#define P19_CURR_REG   0x19 // Port 19
#define P20_CURR_REG   0x1A // Port 20
#define P21_CURR_REG   0x1A // Port 21
#define P22_CURR_REG   0x1B // Port 22
#define P23_CURR_REG   0x1B // Port 23
#define P24_CURR_REG   0x1C // Port 24
#define P25_CURR_REG   0x1C // Port 25
#define P26_CURR_REG   0x1D // Port 26
#define P27_CURR_REG   0x1D // Port 27
#define P28_CURR_REG   0x1E // Port 28
#define P29_CURR_REG   0x1E // Port 29
#define P30_CURR_REG   0x1F // Port 30

#define LED_ON         true
#define LED_OFF        false

class Led_MAX6956
{
  public:
    Led_MAX6956();

    boolean  begin   ( void );
    void toggleLEDs_and_dark( void );
    void LEDsOff( void );
    void ledOff( uint8_t lednum );
    void ledOn( uint8_t lednum );
    void setBrightness( uint8_t lednum, uint8_t brightness );
    void RGBLedOff( uint8_t lednum );
    void RGBLedOn( uint8_t lednum );
    void setRGBLedBrightness( uint8_t lednum, uint8_t brightness );
    
    uint8_t getNumberOfLEDpatterns( void );
    uint8_t getCurrentLEDpattern( void );

    boolean isButton1Pressed ( void );
    boolean isButton2Pressed ( void );
    boolean isCharging ( void );
        
  private:
    boolean  initLeds( void );

    typedef struct
    {
      uint8_t lednum;
      char *ledname;
      uint8_t partnumber;
      uint8_t portnum;
      uint8_t ledreg;
      uint8_t currentreg;
      uint8_t brightness;
      boolean ledState;
      uint16_t ontime;
    } Led;

    Led ledArray[NUMBER_OF_LEDS];
    Led RGBledArray[3];

    boolean  _initialized;
    uint8_t  ledPattern;
};

#endif
