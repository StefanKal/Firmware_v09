/*
Dyno V7.0 looks like
*/

#include <RFduinoBLE.h>
#include <Wire.h>
#include "Led_Max6956.h"

#define PIN_WIRE_SDA         5
#define PIN_WIRE_SCL         6
#define POWER_BUTTON         4
#define START_BUTTON         3

Led_MAX6956 LedDrv;

// debounce time (in ms)
int debounce_time = 10;

// maximum debounce timeout (in ms)
int debounce_timeout = 100;

byte startButtonState;

/*
* We have three structs, each with a leading byte.
* 0 in the leading byte implies an infoStruct
* 1 in the leading byte implies a detectorStruct
* 2 in the leading byte implies an irStruct
*/

// It is worth noting that 3 bytes are added to a struct
// if a struct has more than 1 member. This behavior appears to be platform-independent and
// needs to be tested further for stability.

typedef struct
{
  byte infoByte; 									// 1 byte
  byte LEDpattern;       					// 1 byte
  unsigned short sensor_10mm;    	// 2 bytes
  byte gain_10mm;									// 1 byte
  byte intTime_10mm;							// 1 byte
  unsigned short sensor_20mm;    	// 2 bytes
  byte gain_20mm;									// 1 byte
  byte intTime_20mm;							// 1 byte
  unsigned short sensor_30mm;    	// 2 bytes
  byte gain_30mm;									// 1 byte
  byte intTime_30mm;							// 1 byte
  unsigned short sensor_40mm;    	// 2 bytes
  byte gain_40mm;									// 1 byte
  byte intTime_40mm;							// 1 byte
  // 2 bytes left
} detector_packet;

// IR Values Struct
typedef struct {

  byte infoByte;
  unsigned short ir_10mm;
  unsigned short ir_20mm;
  unsigned short ir_30mm;
  unsigned short ir_40mm;

} ir_packet;

// Info data packet
typedef struct {
  byte infoByte;									// 1 byte
  byte SDCardStatus;              // 1 byte
  int time; 											// 4 bytes
  float cellVoltage; 							// 4 bytes
  float stateOfCharge; 						// 4 bytes
  unsigned short temp_skin;      	// 2 bytes
  unsigned short temp_amb;       	// 2 bytes
  // 3 bytes left
} info_packet;



void setup()
{
  boolean stat;
  // Start I2C, serial, and BLE stack
  Wire.beginOnPins(PIN_WIRE_SCL, PIN_WIRE_SDA);

  // Cannot write to Serial while writing to the SD card, might corrupt the SD card otherwise
  Serial.begin(9600);

  //RFduinoBLE.advertisementData = "temp";
  //RFduinoBLE.deviceName = "Strive";
  //RFduinoBLE.begin();

  // Hardware test
  Serial.println("Starting Hardware Test!");

  delay(1000);

  stat = LedDrv.begin();
  if (stat) {
    Serial.println("LED driver OK");
  }
  Serial.println(" ");

  //Buttons
  pinMode(POWER_BUTTON, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  startButtonState = 0;

  // 680 nm
  LedDrv.setBrightness(LED1, 0x55);       // 0xXF = LED1, 0xFX = LED2
  // 680 and 780 nm
  LedDrv.setBrightness(LED3, 0xF5);       // Led 3 and 4
  // 780 nm
  LedDrv.setBrightness(LED5, 0xFF);       // Led 5 and 6

  LedDrv.setRGBLedBrightness(RED_LED, 0x00);
  LedDrv.setRGBLedBrightness(GREEN_LED, 0x00);
  LedDrv.setRGBLedBrightness(BLUE_LED, 0x00);


  //Batt.reset();
  //Batt.quickStart();

  // Startup animation
  LedDrv.ledOn(LED1);
  delay(100);
  LedDrv.ledOff(LED1);
  LedDrv.ledOn(LED2);
  delay(100);
  LedDrv.ledOff(LED2);
  LedDrv.ledOn(LED3);
  delay(100);
  LedDrv.ledOff(LED3);
  LedDrv.ledOn(LED2);
  delay(100);
  LedDrv.ledOff(LED2);
  LedDrv.ledOn(LED1);
  delay(100);
  LedDrv.ledOff(LED1);

}



// MAIN LOOP HERE
void loop()
{
  detector_packet detectorStruct;
  info_packet infoStruct;
  ir_packet irStruct;

  if (LedDrv.isButton1Pressed())
    LedDrv.RGBLedOn(GREEN_LED);
  else
    LedDrv.RGBLedOff(GREEN_LED);


  // toggle 660nm and  855 nm LEDs
  LedDrv.toggleLEDs_and_dark();

  if (startButtonState == 1)
    delay(100);   // during workout
  else
    delay(200);   // before/after workout

  if (digitalRead(POWER_BUTTON) == 0)
  {
    Serial.println("Powerbutton pressed!");
    Serial.println("Going to sleep ... zzzz");
    LedDrv.LEDsOff();
    delay_until_button(LOW);
    delay(500);

  }
  if (digitalRead(START_BUTTON) == 0)
  {
    if (startButtonState == 0)
    {
      startButtonState = 1;   // workout running
      Serial.println("Startbutton pressed!");
      // Startup animation
      LedDrv.LEDsOff();
      LedDrv.ledOn(LED1);
      delay(100);
      LedDrv.ledOff(LED1);
      LedDrv.ledOn(LED2);
      delay(100);
      LedDrv.ledOff(LED2);
      LedDrv.ledOn(LED3);
      delay(100);
      LedDrv.ledOff(LED3);
      LedDrv.ledOn(LED2);
      delay(100);
      LedDrv.ledOff(LED2);
      LedDrv.ledOn(LED1);
      delay(100);
      LedDrv.ledOff(LED1);
    }
    else if (startButtonState == 1)
    {
      startButtonState = 0;   // workout stopped
      Serial.println("Startbutton pressed!");
      // Stop workout animation
      LedDrv.LEDsOff();
      LedDrv.ledOn(LED3);
      delay(100);
      LedDrv.ledOff(LED3);
      LedDrv.ledOn(LED2);
      delay(100);
      LedDrv.ledOff(LED2);
      LedDrv.ledOn(LED1);
      delay(100);
      LedDrv.ledOff(LED1);
    }
    delay(200);
  }

  unsigned short det1FS =  random(65000);
  unsigned short det2FS = random(65000);
  unsigned short det3FS = random(65000);
  unsigned short det4FS = random(65000);
  unsigned short det1IR = random(65000);
  unsigned short det2IR = random(65000);
  unsigned short det3IR = random(65000);
  unsigned short det4IR = random(65000);

  irStruct.infoByte = 2;
  irStruct.ir_10mm = det1IR;
  irStruct.ir_20mm = det2IR;
  irStruct.ir_30mm = det3IR;
  irStruct.ir_40mm = det4IR;

  detectorStruct.infoByte = 1;
  detectorStruct.LEDpattern = LedDrv.getCurrentLEDpattern();
  detectorStruct.sensor_10mm = det1FS;
  detectorStruct.sensor_20mm = det2FS;
  detectorStruct.sensor_30mm = det3FS;
  detectorStruct.sensor_40mm = det4FS;


  /// --- make some space in the data package and send the IR signal values as well via Bluetooth ---

  infoStruct.infoByte = 0;
  infoStruct.time = millis();
  infoStruct.cellVoltage = 3.7;
  infoStruct.stateOfCharge = 0.96;
  infoStruct.temp_skin = 20;
  infoStruct.temp_amb = RFduino_temperature(CELSIUS);



  // Send all structs in succession, eliminating the need for a second loop
  // It is important that we send these in this order because of the infoByte
  RFduinoBLE.send((char *)&infoStruct, sizeof(infoStruct));
  RFduinoBLE.send((char *)&detectorStruct, sizeof(detectorStruct));
  RFduinoBLE.send((char *)&irStruct, sizeof(irStruct));


}


void RFduinoBLE_onConnect() {
  Serial.println("BLE connected");
  // Startup animation
  LedDrv.ledOn(LED1);
  delay(100);
  LedDrv.ledOff(LED1);
  LedDrv.ledOn(LED2);
  delay(100);
  LedDrv.ledOff(LED2);
  LedDrv.ledOn(LED3);
  delay(100);
  LedDrv.ledOff(LED3);
  LedDrv.ledOn(LED2);
  delay(100);
  LedDrv.ledOff(LED2);
  LedDrv.ledOn(LED1);
  delay(100);
  LedDrv.ledOff(LED1);
}

void RFduinoBLE_onDisconnect() {
  Serial.println("BLE disconnected");
}

int debounce(int state)
{
  int start = millis();
  int debounce_start = start;

  while (millis() - start < debounce_timeout)
    if (digitalRead(POWER_BUTTON) == state)
    {
      if (millis() - debounce_start >= debounce_time)
        return 1;
    }
    else
    {
      debounce_start = millis();
    }
  return 0;
}

int delay_until_button(int state)
{
  // set button edge to wake up on
  if (state)
    RFduino_pinWake(POWER_BUTTON, HIGH);
  else
    RFduino_pinWake(POWER_BUTTON, LOW);

  do
    // switch to lower power mode until a button edge wakes us up
    RFduino_ULPDelay(INFINITE);
  while (! debounce(state));

  // if multiple buttons were configured, this is how you would determine what woke you up
  if (RFduino_pinWoke(POWER_BUTTON))
  {
    // execute code here
    RFduino_resetPinWake(POWER_BUTTON);
    Serial.println("wake up and continue blinking");
  }
}

