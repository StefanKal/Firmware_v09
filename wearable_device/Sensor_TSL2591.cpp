/* TSL2591.cpp
written for the TSL2591 High Dynamic Range Digital Light Sensor
Stefan Kalchmair 04/2015
*/

//#include <avr/pgmspace.h>
//#include <stdlib.h>

#include "Sensor_TSL2591.h"

Sensor_TSL2591::Sensor_TSL2591(void)
{
  // can't use wire here, since wire is not initialized yet
  _initialized = false;
  // initialize past value array
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)  {
    for (uint8_t iLEDpattern = 0; iLEDpattern < MAXIMUM_NUMBER_OF_LED_PATTERNS; iLEDpattern++)  {
      for (uint8_t iPastVal = 0; iPastVal < NUMBER_OF_PAST_SIGNAL_VALUES; iPastVal++)    {
        _pastSigValue[iSens][iLEDpattern][iPastVal] = 0;
      }
      _recordedPastSigValues[iSens][iLEDpattern] = 0;
    }
  }
  selectedSensor = 0;
  currentLEDpattern = 0;
}

boolean Sensor_TSL2591::begin( void )
{
  // activate I2C multiplexer
  pinMode (PIN_I2C_MUX_RESET, OUTPUT);
  digitalWrite(PIN_I2C_MUX_RESET, HIGH);
  Wire.beginOnPins(PIN_WIRE_SCL, PIN_WIRE_SDA);
  Serial.println("I2C MUX activated");

  uint8_t numSensors = scanForSensors();
  Serial.print(numSensors); Serial.println(" TSL2591 sensors found");
  if (numSensors == 0)  return false;

  _initialized = true;

  uint8_t initialGain = 3;              // high gain
  uint8_t initialIntegrationTime = 0;   // 600ms integration time
  // Set default integration time and gain for all sensors
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)  {
    selectSensor(iSens);
    selectGain(initialGain);
    selectIntegrationTime(initialIntegrationTime);
    for (uint8_t iLEDpattern = 0; iLEDpattern < MAXIMUM_NUMBER_OF_LED_PATTERNS; iLEDpattern++)  {
      currentLEDpattern = iLEDpattern;
      gainIndex[selectedSensor][currentLEDpattern] = initialGain;
      integrationTimeIndex[selectedSensor][currentLEDpattern] = initialIntegrationTime;
    }
  }
  currentLEDpattern = 0;
  // Leave device in power down mode on bootup
  return true;
}

// switch I2C_MUX to the desired sensor
boolean  Sensor_TSL2591::selectSensor( uint8_t sensorSelect )
{
  // select desired detector
  switch (sensorSelect)
  {
    case 0 :
      Wire.beginTransmission(MUX_PCA9548ADDR);
      Wire.write(MUX_SENSOR1);
      Wire.endTransmission();
      selectedSensor = sensorSelect;
      break;
    case 1 :
      Wire.beginTransmission(MUX_PCA9548ADDR);
      Wire.write(MUX_SENSOR2);
      Wire.endTransmission();
      selectedSensor = sensorSelect;
      break;
    case 2 :
      Wire.beginTransmission(MUX_PCA9548ADDR);
      Wire.write(MUX_SENSOR3);
      Wire.endTransmission();
      selectedSensor = sensorSelect;
      break;
    case 3 :
      Wire.beginTransmission(MUX_PCA9548ADDR);
      Wire.write(MUX_SENSOR4);
      Wire.endTransmission();
      selectedSensor = sensorSelect;
      break;
    default:
      Wire.beginTransmission(MUX_PCA9548ADDR);
      Wire.write(0x00);  // no sensor selected
      Wire.endTransmission();
      Serial.println("Error: Illegal sensor number");
      return false;
  }
  return true;
}

// switch sensor to the desired gain value
boolean Sensor_TSL2591::selectGain (uint8_t gainselect)
{
  // set detector gain
  switch (gainselect)
  {
    case 0 :
      setGain(TSL2591_GAIN_LOW);
      gainIndex[selectedSensor][currentLEDpattern] = gainselect;
      break;
    case 1 :
      setGain(TSL2591_GAIN_MED);
      gainIndex[selectedSensor][currentLEDpattern] = gainselect;
      break;
    case 2 :
      setGain(TSL2591_GAIN_HIGH);
      gainIndex[selectedSensor][currentLEDpattern] = gainselect;
      break;
    case 3 :
      setGain(TSL2591_GAIN_MAX);
      gainIndex[selectedSensor][currentLEDpattern] = gainselect;
      break;
    default:
      Serial.print("Gain select index out of bounds");
      return true;
  }
  return false;
}

// switch sensor to the desired gain value
boolean Sensor_TSL2591::selectIntegrationTime (uint8_t integrationSelect)
{
  // set detector integration time
  switch (integrationSelect)
  {
    case 0 :
      setIntegrationTime(TSL2591_INTEGRATIONTIME_100MS);
      integrationTimeIndex[selectedSensor][currentLEDpattern] = integrationSelect;
      break;
    case 1 :
      setIntegrationTime(TSL2591_INTEGRATIONTIME_200MS);
      integrationTimeIndex[selectedSensor][currentLEDpattern] = integrationSelect;
      break;
    case 2 :
      setIntegrationTime(TSL2591_INTEGRATIONTIME_300MS);
      integrationTimeIndex[selectedSensor][currentLEDpattern] = integrationSelect;
      break;
    case 3 :
      setIntegrationTime(TSL2591_INTEGRATIONTIME_400MS);
      integrationTimeIndex[selectedSensor][currentLEDpattern] = integrationSelect;
      break;
    case 4 :
      setIntegrationTime(TSL2591_INTEGRATIONTIME_500MS);
      integrationTimeIndex[selectedSensor][currentLEDpattern] = integrationSelect;
      break;
    case 5 :
      setIntegrationTime(TSL2591_INTEGRATIONTIME_600MS);
      integrationTimeIndex[selectedSensor][currentLEDpattern] = integrationSelect;
      break;
    default:
      Serial.print("Integration time select index out of bounds");
      return true;
  }
  return false;
}

// return selected sensor index
uint8_t  Sensor_TSL2591::getSelectedSensor( void )
{
  return selectedSensor;
}
// return current LED pattern
uint8_t  Sensor_TSL2591::getCurrentLEDpattern( void )
{
  return currentLEDpattern;
}

// return gain index
uint8_t  Sensor_TSL2591::getGainIndex( uint8_t sensorSelect )
{
  return gainIndex[sensorSelect][currentLEDpattern];
}

// return integration time index
uint8_t  Sensor_TSL2591::getIntegrationTimeIndex( uint8_t sensorSelect )
{
  return integrationTimeIndex[sensorSelect][currentLEDpattern];
}

void Sensor_TSL2591::enable(void)
{
  if (!_initialized) {
    Serial.println("Error: Sensor not initialized");
    return;
  }
  write8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_ENABLE, TSL2591_ENABLE_POWERON | TSL2591_ENABLE_AEN | TSL2591_ENABLE_AIEN);     // Enable the device by setting the control bit to 0x01
}

void Sensor_TSL2591::disable(void)
{
  if (!_initialized) {
    Serial.println("Error: Sensor not initialized");
    return;
  }
  write8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_ENABLE, TSL2591_ENABLE_POWEROFF);       // Disable the device by setting the control bit to 0x00
}

// set gain for selected sensor
void Sensor_TSL2591::setGain(tsl2591Gain_t gain)
{
  _gain[selectedSensor][currentLEDpattern] = gain;
  write8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL, _integrationTime[selectedSensor][currentLEDpattern] | gain);
}

// get gain for selected sensor
tsl2591Gain_t Sensor_TSL2591::getGain()
{
  return _gain[selectedSensor][currentLEDpattern];
}

// set integration time for selected sensor
void Sensor_TSL2591::setIntegrationTime(tsl2591IntegrationTime_t integrationTime)
{
  _integrationTime[selectedSensor][currentLEDpattern] = integrationTime;
  write8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL, integrationTime | _gain[selectedSensor][currentLEDpattern]);
}

tsl2591IntegrationTime_t Sensor_TSL2591::getIntegrationTime()
{
  return _integrationTime[selectedSensor][currentLEDpattern];
}

// read 8 bit from sensor
uint8_t Sensor_TSL2591::read8(uint8_t reg)
{
  uint8_t data8 = 0;

  Wire.beginTransmission(TSL2591_ADDR);
  Wire.write(0x80 | 0x20 | reg); // command bit, normal mode
  Wire.endTransmission();

  Wire.requestFrom(TSL2591_ADDR, 1);
  if (Wire.available() == 1)  {
    data8 = Wire.read();
  }
  else  {
    Serial.println(" Error in function read8");
  }
  return data8;
}

// read 16 bit from sensor
uint16_t Sensor_TSL2591::read16(uint8_t reg)
{
  uint16_t dataHighByte = 0;
  uint16_t dataLowByte  = 0;
  uint16_t data16       = 0;

  Wire.beginTransmission(TSL2591_ADDR);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(TSL2591_ADDR, 2);
  if (Wire.available() == 2)  {
    dataLowByte = Wire.read();
    dataHighByte = Wire.read();
  }  else
  {
    Serial.println(" Error in function read16");
  }

  dataHighByte <<= 8;
  data16 = dataHighByte | dataLowByte;
  return data16;
}

// read 32 bit from sensor
uint32_t Sensor_TSL2591::read32(uint8_t reg)
{
  uint32_t dataByte1 = 0, dataByte2 = 0, dataByte3 = 0, dataByte4 = 0;
  uint32_t data32    = 0;

  Wire.beginTransmission(TSL2591_ADDR);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(TSL2591_ADDR, 4);
  if (Wire.available() == 4)  {
    dataByte1 = Wire.read();
    dataByte2 = Wire.read();
    dataByte3 = Wire.read();
    dataByte4 = Wire.read();
  }
  else  {
    Serial.println(" Error in function read32");
  }

  dataByte2 <<= 8;
  dataByte3 <<= 16;
  dataByte4 <<= 24;
  data32 = dataByte4 | dataByte3 | dataByte2 | dataByte1;
  return data32;
}

// write 8 bit from sensor
void Sensor_TSL2591::write8 (uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(TSL2591_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// start the data acquisition for all sensors
void Sensor_TSL2591::startAcquisition( uint8_t LEDpattern )
{
  Serial.println("--- Start data acquisition ---");
  currentLEDpattern = LEDpattern;
  uint8_t maxIntegrationTimeIndex = 0;
  // set gain/integration times
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)
  {
    selectSensor(iSens);
    selectGain(gainIndex[iSens][currentLEDpattern]);
    selectIntegrationTime(integrationTimeIndex[iSens][currentLEDpattern]);

    if (integrationTimeIndex[iSens][currentLEDpattern] > maxIntegrationTimeIndex)  {    // find maximum integration time
      maxIntegrationTimeIndex = integrationTimeIndex[iSens][currentLEDpattern];
    }
    Serial.print("iSens= "); Serial.print(iSens); Serial.print(" currentLEDpattern= "); Serial.print(currentLEDpattern);  Serial.print("  gain/iTime = "); Serial.print(gainIndex[iSens][currentLEDpattern]); Serial.print(" / ");  Serial.println(integrationTimeIndex[iSens][currentLEDpattern]);
  }
  // enable all sensors
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)
  {
    selectSensor(iSens);
    enable();
  }
  // Wait x ms for ADC to complete
  for (uint8_t d = 1; d <= maxIntegrationTimeIndex + 1; d++)  {
    delay(110);
  }

  uint32_t sensorSignal_FS_IR;
  // disable oscillator, then read data
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)
  {
    selectSensor(iSens);
    disable();
    // read channel 0 and channel 1 simultaneously
    sensorSignal_FS_IR = read32(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CHAN0_LOW);
    _fullSpecSignal[iSens] = sensorSignal_FS_IR & 0xFFFF;
    _IRSpecSignal[iSens] = (sensorSignal_FS_IR >> 16) & 0xFFFF;
  }

  // save new value into first cell of array of past values, shift all other array values back
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)  {
    for (uint8_t iPastVal = NUMBER_OF_PAST_SIGNAL_VALUES - 1; iPastVal > 0; iPastVal--)  {
      _pastSigValue[iSens][currentLEDpattern][iPastVal] = _pastSigValue[iSens][currentLEDpattern][iPastVal - 1];
    }
    _pastSigValue[iSens][currentLEDpattern][0] = _fullSpecSignal[iSens];

    _recordedPastSigValues[iSens][currentLEDpattern]++;   // increase recorded signal values counter
    if (_recordedPastSigValues[iSens][currentLEDpattern] > NUMBER_OF_PAST_SIGNAL_VALUES) _recordedPastSigValues[iSens][currentLEDpattern] = NUMBER_OF_PAST_SIGNAL_VALUES;
  }

  // display past array
  Serial.println("- past signal value buffer -");
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)  {
    for (uint8_t iPastVal = 0; iPastVal < NUMBER_OF_PAST_SIGNAL_VALUES; iPastVal++)    {
      Serial.print(_pastSigValue[iSens][currentLEDpattern][iPastVal]); Serial.print(" / ");
    }
    Serial.print(" # valid signal values in buffer= "); Serial.print(_recordedPastSigValues[iSens][currentLEDpattern]);
    Serial.println(" ");
  }
}

// auto-adjust gain based on last measurements
boolean Sensor_TSL2591::autoAdjustGain( void )
{
  Serial.println("--- auto-adjust gain/integrationTime ---");
  for (uint8_t iSens = 0; iSens < NUMBER_OF_SENSORS; iSens++)
  {
    if (_recordedPastSigValues[iSens][currentLEDpattern] >= NUMBER_OF_PAST_SIGNAL_VALUES)
    {
      // average over values in past signal value array
      float pastValAvg = 0;
      for (uint8_t iPastVal = 0; iPastVal < NUMBER_OF_PAST_SIGNAL_VALUES; iPastVal++)  {
        pastValAvg += _pastSigValue[iSens][currentLEDpattern][iPastVal];
      }
      pastValAvg /= NUMBER_OF_PAST_SIGNAL_VALUES;
      Serial.print("pastValAvg(iSens="); Serial.print(iSens); Serial.print(" /LEDpatt= "); Serial.print(currentLEDpattern); Serial.print("): "); Serial.println(pastValAvg);

      if (isOverflow(pastValAvg + AUTO_GAIN_SWITCH_BUFFER, integrationTimeIndex[iSens][currentLEDpattern]) == true)  {          // if overflow then switch down. Check only full spectrum since this detector is more sensitive
        gainIntTimeDown(iSens);        // turn down gain or integration time
        _recordedPastSigValues[iSens][currentLEDpattern] = 0;  // clear the number of recorded past values and start filling the past signal buffer again
      }
      else
      {
        float m = SwitchUpMultiplier(iSens);
        float predictedSwitchUpValue = pastValAvg * m;
        Serial.print("Switch up? SwitchUpMultiplier= "); Serial.print(m); Serial.print(" predictedSwitchUpValue= "); Serial.println(predictedSwitchUpValue);
        if (isOverflow(predictedSwitchUpValue + AUTO_GAIN_SWITCH_BUFFER, SwitchUpIntegrationTime(iSens)) == false) {             // if switched up value will fall within the dynamic range, switch gain/inTime up
          gainIntTimeUp(iSens);                    // increase gain or integration time
          _recordedPastSigValues[iSens][currentLEDpattern] = 0;  // clear the number of recorded past values and start filling the past signal buffer again
        }
      }
    }
  }
}

// reduce signal by switching gain or integration time down
void Sensor_TSL2591::gainIntTimeDown ( uint8_t sensorSelect )
{
  uint8_t new_iGI = calc_iGI(gainIndex[sensorSelect][currentLEDpattern], integrationTimeIndex[sensorSelect][currentLEDpattern], -1);
  uint8_t newGainIndex = calcNewGainIndex(new_iGI);
  uint8_t newIntegrationTimeIndex = calcNewIntegrationTimeIndex(new_iGI);
  Serial.print("Sens"); Serial.print(sensorSelect); Serial.print("  <--- switch down from G/I= ");
  Serial.print(gainIndex[sensorSelect][currentLEDpattern]); Serial.print("/");  Serial.print(integrationTimeIndex[sensorSelect][currentLEDpattern]);
  Serial.print(" to G/I= "); Serial.print(newGainIndex); Serial.print("/");  Serial.println(newIntegrationTimeIndex);

  // now switch to new gain values
  selectSensor(sensorSelect);
  selectGain(newGainIndex);
  selectIntegrationTime(newIntegrationTimeIndex);
}

// calculate the multiplication factor on the signal value when switching gain/intTime one step up
float Sensor_TSL2591::SwitchUpMultiplier ( uint8_t sensorSelect )
{
  uint8_t iGI = calc_iGI(gainIndex[sensorSelect][currentLEDpattern], integrationTimeIndex[sensorSelect][currentLEDpattern], 0);
  uint8_t new_iGI = calc_iGI(gainIndex[sensorSelect][currentLEDpattern], integrationTimeIndex[sensorSelect][currentLEDpattern], 1);
  return (float) GainIntegrationProduct[new_iGI] / GainIntegrationProduct[iGI];
}

// calculate the integration time after a switch up
uint8_t Sensor_TSL2591::SwitchUpIntegrationTime ( uint8_t sensorSelect )
{
  uint8_t new_iGI = calc_iGI(gainIndex[sensorSelect][currentLEDpattern], integrationTimeIndex[sensorSelect][currentLEDpattern], 1);
  uint8_t newIntegrationTimeIndex = calcNewIntegrationTimeIndex(new_iGI);
  return (float) newIntegrationTimeIndex;
}

// increase signal by switching gain or integration time down
void Sensor_TSL2591::gainIntTimeUp ( uint8_t sensorSelect )
{
  uint8_t new_iGI = calc_iGI(gainIndex[sensorSelect][currentLEDpattern], integrationTimeIndex[sensorSelect][currentLEDpattern], 1);
  uint8_t newGainIndex = calcNewGainIndex(new_iGI);
  uint8_t newIntegrationTimeIndex = calcNewIntegrationTimeIndex(new_iGI);
  Serial.print("Sens"); Serial.print(sensorSelect); Serial.print("  ---> switch up from G/I= ");
  Serial.print(gainIndex[sensorSelect][currentLEDpattern]); Serial.print("/");  Serial.print(integrationTimeIndex[sensorSelect][currentLEDpattern]);
  Serial.print(" to G/I= "); Serial.print(newGainIndex); Serial.print("/");  Serial.println(newIntegrationTimeIndex);

  // now switch to new gain values
  selectSensor(sensorSelect);
  selectGain(newGainIndex);
  selectIntegrationTime(newIntegrationTimeIndex);
}

// calculate next combined gainIntegrationTimeValue
uint8_t Sensor_TSL2591::calc_iGI ( uint8_t iGain, uint8_t iIntTime, int indexStep)
{
  uint8_t iGI;
  short new_iGI;  //needs to allow negative numbers
  iGI = iGain * 6 + iIntTime;                                                // generate combined gain/integrationTime index, max value is 23 ([0-3] gain x [0-5] intTime)
  new_iGI = iGI + indexStep;                                                 // switch N steps up or down
  if (new_iGI <= 0) new_iGI = 0;                                             // stop if already on lowest gain
  if (new_iGI >= 23) new_iGI = 23;                                           // stop if already on highest gain
  return (uint8_t) new_iGI;
}

// calculate new gain index
uint8_t Sensor_TSL2591::calcNewGainIndex ( uint8_t iGI)
{
  uint8_t newGainIndex;
  newGainIndex = (uint8_t) iGI / 6;                                      // extract new gain index
  return newGainIndex;
}

// calculate new integration time index
uint8_t Sensor_TSL2591::calcNewIntegrationTimeIndex ( uint8_t iGI)
{
  uint8_t newGainIndex, newIntegrationTimeIndex;
  newGainIndex = (uint8_t) iGI / 6;                                      // extract new gain index
  newIntegrationTimeIndex = (uint8_t) iGI - newGainIndex * 6;            // extract new integration time index
  return newIntegrationTimeIndex;
}

// get the full spectrum data
uint16_t Sensor_TSL2591::getFullSpecSignal( uint8_t sensorSelect )
{
  return _fullSpecSignal[sensorSelect];
}

// get the IR spectrum data
uint16_t Sensor_TSL2591::getIRSpecSignal( uint8_t sensorSelect )
{
  return _IRSpecSignal[sensorSelect];
}

// check for an overflow
boolean Sensor_TSL2591::isOverflow(float sensorVal, uint8_t intTimeIndex)
{
  boolean overflowFlag = false;
  // Check for overflow conditions
  if (intTimeIndex == 0)  // integration time = 100 ms?
  {
    if (sensorVal >= 0x9400)
    {
      overflowFlag = true;
    }
  }
  else // any other integration time
  {
    if (sensorVal >= 0xFFFF)
    {
      overflowFlag = true;
    }
  }
  return overflowFlag;
}

// scan for sensors and return number of sensor
uint8_t Sensor_TSL2591::scanForSensors (void)
{
  uint8_t numSensors = 0;
  uint8_t id = 0;
  uint8_t sensSelectByte = 0;

  for (uint8_t ii = 0; ii < NUMBER_OF_SENSORS; ii++)
  {
    sensSelectByte = 1 << ii;       // Select photodetector by writing a '1' onto the select bit
    Wire.beginTransmission(MUX_PCA9548ADDR);
    Wire.write(sensSelectByte);
    Wire.endTransmission();
    id = read8(0x12);
    if (id == 0x50 )    numSensors++;
  }

  return numSensors;
}




