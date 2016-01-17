/* TSL2591.h
written for the TSL2591 High Dynamic Range Digital Light Sensor
Stefan Kalchmair 04/2015
*/

#ifndef _SENSOR_TSL2591_H_
#define _SENSOR_TSL2591_H_

#include <Wire.h>

#define NUMBER_OF_SENSORS               4       // Number of sensors on PCB
#define MAXIMUM_NUMBER_OF_LED_PATTERNS  5       // Maximum number of LED illumination patterns: e.g. all 680 nm LEDs, one 810 nm LED and dark measurement
#define NUMBER_OF_PAST_SIGNAL_VALUES    3      // Number of past signal values to average before making a gain/iTime switch decision
#define AUTO_GAIN_SWITCH_BUFFER      5000     // when switching to a different gain/intTime, leave some space to make sure next value will be smaller than maximum


// I2C multiplexer
#define PIN_I2C_MUX_RESET    4
#define MUX_PCA9548ADDR      0x70     //1110100
#define MUX_SENSOR1          0x01
#define MUX_SENSOR2          0x02
#define MUX_SENSOR3          0x04
#define MUX_SENSOR4          0x08

#define PIN_WIRE_SDA         5
#define PIN_WIRE_SCL         6

// TSD2591 registers
#define TSL2591_VISIBLE           2       // channel 0 - channel 1
#define TSL2591_INFRARED          1       // channel 1
#define TSL2591_FULLSPECTRUM      0       // channel 0

#define TSL2591_ADDR              0x29
#define TSL2591_READBIT           0x01

#define TSL2591_COMMAND_BIT       0xA0    // bits 7 and 5 for 'command normal'
#define TSL2591_CLEAR_BIT         0x40    // Clears any pending interrupt (write 1 to clear)
#define TSL2591_WORD_BIT          0x20    // 1 = read/write word (rather than byte)
#define TSL2591_BLOCK_BIT         0x10    // 1 = using block read/write

#define TSL2591_ENABLE_POWERON    0x01
#define TSL2591_ENABLE_POWEROFF   0x00
#define TSL2591_ENABLE_AEN        0x02
#define TSL2591_ENABLE_AIEN       0x10

#define TSL2591_CONTROL_RESET     0x80

enum
{
  TSL2591_REGISTER_ENABLE           = 0x00,
  TSL2591_REGISTER_CONTROL          = 0x01,
  TSL2591_REGISTER_THRESHHOLDL_LOW  = 0x02,
  TSL2591_REGISTER_THRESHHOLDL_HIGH = 0x03,
  TSL2591_REGISTER_THRESHHOLDH_LOW  = 0x04,
  TSL2591_REGISTER_THRESHHOLDH_HIGH = 0x05,
  TSL2591_REGISTER_INTERRUPT        = 0x06,
  TSL2591_REGISTER_CRC              = 0x08,
  TSL2591_REGISTER_ID               = 0x0A,
  TSL2591_REGISTER_CHAN0_LOW        = 0x14,
  TSL2591_REGISTER_CHAN0_HIGH       = 0x15,
  TSL2591_REGISTER_CHAN1_LOW        = 0x16,
  TSL2591_REGISTER_CHAN1_HIGH       = 0x17
};

typedef enum
{
  TSL2591_INTEGRATIONTIME_100MS     = 0x00,
  TSL2591_INTEGRATIONTIME_200MS     = 0x01,
  TSL2591_INTEGRATIONTIME_300MS     = 0x02,
  TSL2591_INTEGRATIONTIME_400MS     = 0x03,
  TSL2591_INTEGRATIONTIME_500MS     = 0x04,
  TSL2591_INTEGRATIONTIME_600MS     = 0x05,
}
tsl2591IntegrationTime_t;

typedef enum
{
  TSL2591_GAIN_LOW                  = 0x00,    // low gain (1x)
  TSL2591_GAIN_MED                  = 0x10,    // medium gain (25x)
  TSL2591_GAIN_HIGH                 = 0x20,    // medium gain (428x)
  TSL2591_GAIN_MAX                  = 0x30,    // max gain (9876x)
}
tsl2591Gain_t;

const uint32_t GainIntegrationProduct[] = {1 * 100, 1 * 200, 1 * 300, 1 * 400, 1 * 500, 1 * 600, \
                                           25 * 100, 25 * 200, 25 * 300, 25 * 400, 25 * 500, 25 * 600, \
                                           428 * 100, 428 * 200, 428 * 300, 428 * 400, 428 * 500, 428 * 600, \
                                           9876 * 100, 9876 * 200, 9876 * 300, 9876 * 400, 9876 * 500, 9876 * 600 \
                                          };

class Sensor_TSL2591
{
  public:
    Sensor_TSL2591();

    boolean   begin   ( void );
    void      enable  ( void );
    void      disable ( void );
    void      write8  ( uint8_t reg, uint8_t value );
    uint32_t  read32  ( uint8_t reg );
    uint16_t  read16  ( uint8_t reg );
    uint8_t   read8   ( uint8_t reg );

    boolean  selectSensor( uint8_t sensorSelect );
    boolean  selectGain( uint8_t gainSelect );
    boolean  selectIntegrationTime( uint8_t integrationSelect );
    uint8_t  getSelectedSensor( void );
    uint8_t  getCurrentLEDpattern( void );
    uint8_t  getGainIndex( uint8_t sensorSelect );
    uint8_t  getIntegrationTimeIndex( uint8_t sensorSelect );

    uint8_t   scanForSensors ( void );  //return number of found sensors

    boolean   autoAdjustGain( void );    // auto-adjust gain based on last measurements
    void      startAcquisition( uint8_t LEDpattern );  // start the data acquisition for all detectors
    uint16_t  getFullSpecSignal( uint8_t sensorSelect );  // return full spectrum signal for selected photodetector
    uint16_t  getIRSpecSignal( uint8_t sensorSelect );

  private:
    tsl2591IntegrationTime_t  _integrationTime[NUMBER_OF_SENSORS] [MAXIMUM_NUMBER_OF_LED_PATTERNS];
    tsl2591Gain_t             _gain[NUMBER_OF_SENSORS] [MAXIMUM_NUMBER_OF_LED_PATTERNS];
    int32_t                   _sensorID[NUMBER_OF_SENSORS];
    uint16_t                  _IRSpecSignal[NUMBER_OF_SENSORS];
    uint16_t                  _fullSpecSignal[NUMBER_OF_SENSORS];

    uint16_t                  _pastSigValue[NUMBER_OF_SENSORS] [MAXIMUM_NUMBER_OF_LED_PATTERNS] [NUMBER_OF_PAST_SIGNAL_VALUES];
    uint8_t                   _recordedPastSigValues[NUMBER_OF_SENSORS] [MAXIMUM_NUMBER_OF_LED_PATTERNS];

    uint8_t                   selectedSensor;
    uint8_t                   currentLEDpattern;
    uint8_t                   gainIndex[NUMBER_OF_SENSORS] [MAXIMUM_NUMBER_OF_LED_PATTERNS];
    uint8_t                   integrationTimeIndex[NUMBER_OF_SENSORS][MAXIMUM_NUMBER_OF_LED_PATTERNS];

    boolean                   _initialized;

    void                      setGain( tsl2591Gain_t gain );
    void                      setIntegrationTime( tsl2591IntegrationTime_t integrationTime );
    tsl2591IntegrationTime_t  getIntegrationTime();
    tsl2591Gain_t             getGain();
    void                      gainIntTimeDown ( uint8_t sensorSelect );
    void                      gainIntTimeUp ( uint8_t sensorSelect );
    uint8_t                   calc_iGI ( uint8_t iGain, uint8_t iIntTime, int indexStep);
    uint8_t                   calcNewGainIndex ( uint8_t iGI);
    uint8_t                   calcNewIntegrationTimeIndex ( uint8_t iGI);
    float                     SwitchUpMultiplier ( uint8_t sensorSelect );
    uint8_t                   SwitchUpIntegrationTime ( uint8_t sensorSelect );
    boolean                   isOverflow( float sensorVal, uint8_t intTimeIndex );
};

#endif
