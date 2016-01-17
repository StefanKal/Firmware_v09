/*
Dyno V5.0 rev A
Written by: Samuel Huberman, Stefan Kalchmair
Date updated: 6-June-2015
LED driver MAX6956
*/

#include "Led_MAX6956.h"

#define MAX6956_address   0x44

Led_MAX6956::Led_MAX6956(void)
{
  // can't use wire here, since wire is not initialized yet
  _initialized = false;
}

boolean Led_MAX6956::begin(void)
{
  // Set configuration register
  Wire.beginTransmission(MAX6956_address);
  Wire.write(0x04); // configuration register address
  Wire.write(0x41); // Set the shutdown/run bit of the configuration register (aka normal mode??) 0x is global current mode? 4x individual??
  Wire.endTransmission();

  // Configure ports as LED drive mode (Table 1, Table 2, Table 5)
  Wire.beginTransmission(MAX6956_address);
  Wire.write(0x09); // select ports 12-15
  Wire.write(0x55); // set ports  4-7  NOT CONNECTED, set as GPIO output to save power and autoinc
  Wire.write(0x55); // set ports  8-11 NOT CONNECTED, set as GPIO output to save power and autoinc
  Wire.write(0x00); // set ports 12-15 as LED driver and autoinc
  Wire.write(0x00); // set ports 16-19 as LED driver and autoinc
  Wire.write(0x00); // set ports 20-23 as LED driver and autoinc
  Wire.write(0x00); // set ports 24-27 as LED driver and autoinc
  Wire.write(0xFF); // set ports 28-31 as GPIO input with pullup
  Wire.endTransmission();

  // Turn off all LEDs
  for (int i = 1; i < 18; i++) {
    Wire.beginTransmission(MAX6956_address);
    Wire.write(0x2B + i);
    Wire.write(0x00);
    Wire.endTransmission();
  }

  // Set individual current registers for LEDs (Table 11, Table 12): ALL DIM
  for (int i = 1; i < 14; i++) {
    Wire.beginTransmission(MAX6956_address);
    Wire.write(0x12 + i);
    Wire.write(0x00);
    Wire.endTransmission();
  }

  //initialize LED parameters
  initLeds();
  _initialized = true;
  return _initialized;
}

// function to toggle the status of the LEDs
uint8_t Led_MAX6956::getNumberOfLEDpatterns( void )
{
  return NUMBER_OF_LED_PATTERNS;
}

uint8_t Led_MAX6956::getCurrentLEDpattern( void )
{
  return ledPattern;
}

// function to toggle the status of the LEDs
void Led_MAX6956::toggleLEDs_and_dark( void )
{
  if (ledPattern == 0) {
    ledOff(LED1);
    ledOff(LED2);
    ledPattern = 1;
  }
  else if (ledPattern == 1) {
    ledOn(LED1);
    ledOff(LED2);
    ledPattern = 2;
  }
  else {
    ledOff(LED1);
    ledOn(LED2);
    ledPattern = 0;
  }
}

// function to turn LEDs off
void Led_MAX6956::LEDsOff( void )
{
  ledOff(LED1);
  ledOff(LED2);
}

// turn LED off
void Led_MAX6956::ledOff(uint8_t lednum) {
  Wire.beginTransmission(MAX6956_address);
  Wire.write(ledArray[lednum].ledreg);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();
  ledArray[lednum].ledState = LED_OFF;
}

// turn LED on
void Led_MAX6956::ledOn(uint8_t lednum) {
  Wire.beginTransmission(MAX6956_address);
  Wire.write(ledArray[lednum].ledreg);
  Wire.write(0x01);
  Wire.write(0x01);
  Wire.endTransmission();
  ledArray[lednum].ledState = LED_ON;
}

// set LED brightness
void Led_MAX6956::setBrightness(uint8_t lednum, uint8_t brightness ) {
  ledArray[lednum].brightness = brightness;
  Wire.beginTransmission(MAX6956_address);
  Wire.write(ledArray[lednum].currentreg);
  Wire.write(brightness);
  Wire.endTransmission();
}

/* --- RGB LED --- */

// turn RGB LED off
void Led_MAX6956::RGBLedOff( uint8_t lednum ) {
  Wire.beginTransmission(MAX6956_address);
  Wire.write(RGBledArray[lednum].ledreg);
  Wire.write(0x00);
  Wire.endTransmission();
  RGBledArray[lednum].ledState = LED_OFF;
}

// turn red LED on
void Led_MAX6956::RGBLedOn( uint8_t lednum ) {
  Wire.beginTransmission(MAX6956_address);
  Wire.write(RGBledArray[lednum].ledreg);
  Wire.write(0x01);
  Wire.endTransmission();
  RGBledArray[lednum].ledState = LED_ON;
}

// set LED brightness
void Led_MAX6956::setRGBLedBrightness(uint8_t lednum, uint8_t brightness ) {
  RGBledArray[lednum].brightness = brightness;
  Wire.beginTransmission(MAX6956_address);
  Wire.write(RGBledArray[lednum].currentreg);
  Wire.write(brightness);
  Wire.endTransmission();
}

/* --- read from ports (Buttons, charge status pin) --- */
boolean Led_MAX6956::isButton1Pressed ( void ) {
  boolean buttonState = false;
  Wire.beginTransmission(MAX6956_address);    //  Send input register address
  Wire.write(BUTTON1_PORT);
  Wire.endTransmission();

  Wire.requestFrom(MAX6956_address, 1);
  if (Wire.available() == 1)  {
    if (Wire.read() == 0) {     // Pulldown: 0=pressed, 1=not pressed
      buttonState = true;
    }
  }
  return buttonState;
}

boolean Led_MAX6956::isButton2Pressed ( void ) {
  boolean buttonState = false;
  Wire.beginTransmission(MAX6956_address);    //  Send input register address
  Wire.write(BUTTON2_PORT);
  Wire.endTransmission();

  Wire.requestFrom(MAX6956_address, 1);
  if (Wire.available() == 1)  {
    if (Wire.read() == 0) {     // Pulldown: 0=pressed, 1=not pressed
      buttonState = true;
    }
  }
  return buttonState;
}

boolean Led_MAX6956::isCharging ( void ) {
  boolean chargerState = false;
  Wire.beginTransmission(MAX6956_address);    //  Send input register address
  Wire.write(BATTERY_STAT_PORT);
  Wire.endTransmission();

  Wire.requestFrom(MAX6956_address, 1);
  if (Wire.available() == 1)  {
    if (Wire.read() == 0) {     // Pulldown: 0=pressed, 1=not pressed
      chargerState = true;
    }
  }
  return chargerState;
}

// initialize all LEDs
boolean Led_MAX6956::initLeds( void ) {
  // LED1
  ledArray[LED1].lednum = LED1;
  ledArray[LED1].ledname = LED1_NAME;
  ledArray[LED1].portnum = 12;
  ledArray[LED1].ledreg = P12_REG;
  ledArray[LED1].currentreg = P12_CURR_REG;
  ledArray[LED1].brightness = 0x00;
  ledArray[LED1].ledState = LED_OFF;
  ledArray[LED1].ontime = 100;


  // LED2
  ledArray[LED2].lednum = LED2;
  ledArray[LED2].ledname = LED2_NAME;
  ledArray[LED2].portnum = 14;
  ledArray[LED2].ledreg = P14_REG;
  ledArray[LED2].currentreg = P14_CURR_REG;
  ledArray[LED2].brightness = 0x00;
  ledArray[LED2].ledState = LED_OFF;
  ledArray[LED2].ontime = 100;

  /* --- RGB LED --- */
  // red LED
  RGBledArray[RED_LED].lednum = RED_LED;
  RGBledArray[RED_LED].ledname = RED_LED_NAME;
  RGBledArray[RED_LED].portnum = 18;
  RGBledArray[RED_LED].ledreg = P18_REG;
  RGBledArray[RED_LED].currentreg = P24_CURR_REG;
  RGBledArray[RED_LED].brightness = 0x00;
  RGBledArray[RED_LED].ledState = LED_OFF;
  RGBledArray[RED_LED].ontime = 100;

  // green LED
  RGBledArray[GREEN_LED].lednum = GREEN_LED;
  RGBledArray[GREEN_LED].ledname = GREEN_LED_NAME;
  RGBledArray[GREEN_LED].portnum = 19;
  RGBledArray[GREEN_LED].ledreg = P19_REG;
  RGBledArray[GREEN_LED].currentreg = P22_CURR_REG;
  RGBledArray[GREEN_LED].brightness = 0x00;
  RGBledArray[GREEN_LED].ledState = LED_OFF;
  RGBledArray[GREEN_LED].ontime = 100;

  // blue LED
  RGBledArray[BLUE_LED].lednum = BLUE_LED;
  RGBledArray[BLUE_LED].ledname = BLUE_LED_NAME;
  RGBledArray[BLUE_LED].portnum = 20;
  RGBledArray[BLUE_LED].ledreg = P20_REG;
  RGBledArray[BLUE_LED].currentreg = P23_CURR_REG;
  RGBledArray[BLUE_LED].brightness = 0x00;
  RGBledArray[BLUE_LED].ledState = LED_OFF;
  RGBledArray[BLUE_LED].ontime = 100;

  return true;
};
