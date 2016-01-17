/* FuelGauge.h
written for the MAX17043 fuel gauge
Stefan Kalchmair 06/2015
modified from Luca Dentella's code (http://www.lucadentella.it)
*/

#include "FuelGauge.h"

FuelGauge::FuelGauge(void)
{
  // can't use wire here, since wire is not initialized yet
  _initialized = false;
}

boolean FuelGauge::begin(void)
{
  _initialized = true;

  return _initialized;
}

float FuelGauge::getVCell() {

	byte MSB = 0;
	byte LSB = 0;
	
	readRegister(VCELL_REGISTER, MSB, LSB);
	int value = (MSB << 4) | (LSB >> 4);
	return map(value, 0x000, 0xFFF, 0, 50000) / 10000.0;
	//return value * 0.00125;
}

float FuelGauge::getSoC() {
	
	byte MSB = 0;
	byte LSB = 0;
	
	readRegister(SOC_REGISTER, MSB, LSB);
	float decimal = LSB / 256.0;
	return MSB + decimal;	
}

int FuelGauge::getVersion() {

	byte MSB = 0;
	byte LSB = 0;
	
	readRegister(VERSION_REGISTER, MSB, LSB);
	return (MSB << 8) | LSB;
}

byte FuelGauge::getCompensateValue() {

	byte MSB = 0;
	byte LSB = 0;
	
	readConfigRegister(MSB, LSB);
	return MSB;
}

byte FuelGauge::getAlertThreshold() {

	byte MSB = 0;
	byte LSB = 0;
	
	readConfigRegister(MSB, LSB);	
	return 32 - (LSB & 0x1F);
}

void FuelGauge::setAlertThreshold(byte threshold) {

	byte MSB = 0;
	byte LSB = 0;
	
	readConfigRegister(MSB, LSB);	
	if(threshold > 32) threshold = 32;
	threshold = 32 - threshold;
	
	writeRegister(CONFIG_REGISTER, MSB, (LSB & 0xE0) | threshold);
}

boolean FuelGauge::inAlert() {

	byte MSB = 0;
	byte LSB = 0;
	
	readConfigRegister(MSB, LSB);	
	return LSB & 0x20;
}

void FuelGauge::clearAlert() {

	byte MSB = 0;
	byte LSB = 0;
	
	readConfigRegister(MSB, LSB);	
}

void FuelGauge::reset() {
	
	writeRegister(COMMAND_REGISTER, 0x00, 0x54);
}

void FuelGauge::quickStart() {
	
	writeRegister(MODE_REGISTER, 0x40, 0x00);
}


void FuelGauge::readConfigRegister(byte &MSB, byte &LSB) {

	readRegister(CONFIG_REGISTER, MSB, LSB);
}

void FuelGauge::readRegister(byte startAddress, byte &MSB, byte &LSB) {

	Wire.beginTransmission(MAX17043_ADDRESS);
	Wire.write(startAddress);
	Wire.endTransmission();
	
	Wire.requestFrom(MAX17043_ADDRESS, 2);
	MSB = Wire.read();
	LSB = Wire.read();
}

void FuelGauge::writeRegister(byte address, byte MSB, byte LSB) {

	Wire.beginTransmission(MAX17043_ADDRESS);
	Wire.write(address);
	Wire.write(MSB);
	Wire.write(LSB);
	Wire.endTransmission();
}

