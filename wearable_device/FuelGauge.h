/* FuelGauge.h
written for the MAX17043 fuel gauge
Stefan Kalchmair 06/2015
modified from Luca Dentella's code (http://www.lucadentella.it)
*/

#ifndef FUEL_GAUGE_H_
#define FUEL_GAUGE_H_

#include <Wire.h>

#define MAX17043_ADDRESS	0x36

#define VCELL_REGISTER		0x02
#define SOC_REGISTER		0x04
#define MODE_REGISTER		0x06
#define VERSION_REGISTER	0x08
#define CONFIG_REGISTER		0x0C
#define COMMAND_REGISTER	0xFE

class FuelGauge
{
  public:
    FuelGauge();
    boolean   begin ( void );

    float getVCell();
    float getSoC();
    int getVersion();
    byte getCompensateValue();
    byte getAlertThreshold();
    void setAlertThreshold(byte threshold);
    boolean inAlert();
    void clearAlert();

    void reset();
    void quickStart();

  private:
    void readConfigRegister(byte &MSB, byte &LSB);
    void readRegister(byte startAddress, byte &MSB, byte &LSB);
    void writeRegister(byte address, byte MSB, byte LSB);
    boolean _initialized;
};

#endif


