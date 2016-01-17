/*
Firmware for V09
Written by: Daniel Wiese, Samuel Huberman, Stefan Kalchmair, Dharmesh Tarapore
Last updated by: DT
Date updated: Jan 2016
Master code for wearable Strive Lactate Meter
*/

/*
 * SD Card information
 * SD Card Codes:
 * 4: Able to write and currently logging to file
 * 5: Able to write but not currently logging to file
 * 6: Failed to open file for writing
 * 7: Unable to initialize SD Card: Not recoverable
 */
// ============== TODO ======================
// - change port 31 to input, read charging state
// - Send IR Values via BLE
// write parallel sensor acquisition
// define protocol
// add buttons
// Figure out a way to send descriptive error messages over BLE

#include <SPI.h>
#include <SD.h>
#include <RFduinoBLE.h>
#include <Wire.h>
#include "Sensor_Tsl2591.h"
#include "Led_Max6956.h"
#include "FuelGauge.h"

#define PIN_WIRE_SDA         5
#define PIN_WIRE_SCL         6
#define POWER_BUTTON         3
Sd2Card card;
/*
// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
*/

const int chipSelect = 0;
bool shouldSync = false;
// This will help debug errors since writing to the SD card means we can't use the Serial port
// until the next iteration of the Dyno prototype
int sd_card_status = 0;

// We will write to this file
File myFile;

Sensor_TSL2591 Tsl;
Led_MAX6956 LedDrv;
FuelGauge Batt;

// debounce time (in ms)
int debounce_time = 10;

// maximum debounce timeout (in ms)
int debounce_timeout = 100;

char wfilename[30] = "log_0.txt";

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


// Sync Status Struct
typedef struct {
  byte infoByte;
} sync_packet;

typedef struct {
  byte infoByte;
  char fileNamed [15];  
} file_packet;

typedef struct {
  byte infoByte;
  char fileNamed [15];  
} file_packet2;


void setup()
{
  boolean stat;
  // Start I2C, serial, and BLE stack
  Wire.beginOnPins(PIN_WIRE_SCL, PIN_WIRE_SDA);

  // Cannot write to Serial while writing to the SD card, might corrupt the SD card otherwise
  Serial.begin(9600);

  //RFduinoBLE.advertisementData = "temp";
  RFduinoBLE.deviceName = "Strive";
  RFduinoBLE.begin();

  // Hardware test
  Serial.println("Starting Hardware Test!");

  stat = LedDrv.begin();
  if (stat) {
    Serial.println("LED driver OK");
  }
  stat = Tsl.begin();
  if (stat) {
    Serial.println("Light sensors OK");
  }
  stat = Batt.begin();
  if (stat) {
    Serial.println("Fuel gauge OK");
  }
  Serial.println(" ");

  // LED1 = 650 nm
  LedDrv.setBrightness(LED1, 0xFF);       // 0xXF  + 0xFX = LED1
  // LED2 = 855 nm
  LedDrv.setBrightness(LED2, 0x00);

  LedDrv.setRGBLedBrightness(RED_LED, 0x00);
  LedDrv.setRGBLedBrightness(GREEN_LED, 0x00);
  LedDrv.setRGBLedBrightness(BLUE_LED, 0x00);

  Batt.reset();
  Batt.quickStart();

    //Buttons
  pinMode(POWER_BUTTON, INPUT_PULLUP);
  
  // Turn on blue status LED
  LedDrv.RGBLedOn(BLUE_LED);
  delay(600);
  LedDrv.RGBLedOff(BLUE_LED);
  LedDrv.RGBLedOn(RED_LED);
  delay(600);
  LedDrv.RGBLedOff(RED_LED);
  LedDrv.RGBLedOn(GREEN_LED);
  delay(600);
  LedDrv.RGBLedOff(GREEN_LED);

  LedDrv.toggleLEDs_and_dark();
  LedDrv.toggleLEDs_and_dark();
  LedDrv.toggleLEDs_and_dark();
  LedDrv.toggleLEDs_and_dark();

  // Set up the SD card here:
//  pinMode(0, OUTPUT);
//  if (!SD.begin(chipSelect)) {
//    // Could not find an SD Card -- Error 7
//    sd_card_status = 7;
//    //return;
//  }

  // Create the tracker file if it does not exist
  if(!SD.exists("tracker.txt")) {
    File trackerFile = SD.open("tracker.txt", FILE_WRITE);
    if(trackerFile) {
      trackerFile.println("filename = log_0.txt;");
    }
    trackerFile.close();
  }

  
}



// MAIN LOOP HERE
void loop()
{
  if (digitalRead(POWER_BUTTON) == 0)
  {
    Serial.println("Powerbutton pressed!");
    Serial.println("Going to sleep ... zzzz");
    LedDrv.LEDsOff();
    delay_until_button(LOW);
    delay(500);
  }
  
  detector_packet detectorStruct;
  info_packet infoStruct;
  ir_packet irStruct;
  
  if (LedDrv.isButton1Pressed())
    LedDrv.RGBLedOn(GREEN_LED);
  else
    LedDrv.RGBLedOff(GREEN_LED);

  if (LedDrv.isButton2Pressed())
    LedDrv.RGBLedOn(BLUE_LED);
  else
    LedDrv.RGBLedOff(BLUE_LED);

  //if (LedDrv.isCharging())
  // LedDrv.RGBLedOn(RED_LED);
  //else
  //  LedDrv.RGBLedOff(RED_LED);

  float cellVoltage = Batt.getVCell();
  float stateOfCharge = Batt.getSoC();

  // toggle 660nm and  855 nm LEDs
  LedDrv.toggleLEDs_and_dark();
  //  LedDrv.nextLED();

  Tsl.startAcquisition(LedDrv.getCurrentLEDpattern());

  unsigned short det1FS = Tsl.getFullSpecSignal(0);
  unsigned short det2FS = Tsl.getFullSpecSignal(1);
  unsigned short det3FS = Tsl.getFullSpecSignal(2);
  unsigned short det4FS = Tsl.getFullSpecSignal(3);
  unsigned short det1IR = Tsl.getIRSpecSignal(0);
  unsigned short det2IR = Tsl.getIRSpecSignal(1);
  unsigned short det3IR = Tsl.getIRSpecSignal(2);
  unsigned short det4IR = Tsl.getIRSpecSignal(3);

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

  // TODO: casting the data typ of Tsl.getGain and Tsl.getIntTime as bytes, check this later.

  detectorStruct.gain_10mm = Tsl.getGainIndex(0);
  detectorStruct.intTime_10mm = Tsl.getIntegrationTimeIndex(0);
  detectorStruct.gain_20mm = Tsl.getGainIndex(1);
  detectorStruct.intTime_20mm = Tsl.getIntegrationTimeIndex(1);
  detectorStruct.gain_30mm = Tsl.getGainIndex(2);
  detectorStruct.intTime_30mm = Tsl.getIntegrationTimeIndex(2);
  detectorStruct.gain_40mm = Tsl.getGainIndex(3);
  detectorStruct.intTime_40mm = Tsl.getIntegrationTimeIndex(3);

  /// --- make some space in the data package and send the IR signal values as well via Bluetooth ---

  infoStruct.infoByte = 0;
  infoStruct.time = millis();
  infoStruct.cellVoltage = cellVoltage;
  infoStruct.stateOfCharge = stateOfCharge;
  infoStruct.temp_skin = 0;
  infoStruct.temp_amb = RFduino_temperature(CELSIUS);

  Tsl.autoAdjustGain();
  
  
  if(sd_card_status == 4) {

    // Initialize card every time so the firmware does not crash if the SD card is removed while the 
  // loop is running
    if(!card.init(SPI_HALF_SPEED, chipSelect)) {
      sd_card_status = 7;
    }
    // Write to SD card here
     
   else {
        
        myFile = SD.open(wfilename, FILE_WRITE); //wfilename
        if(myFile) {

            // Log in the same format as our OS X App log file
            myFile.printf("\"cell_voltage\" = %f;\n", cellVoltage);
            myFile.printf("\"gain_10mm\" = %d;\n", detectorStruct.gain_10mm);
            myFile.printf("\"gain_20mm\" = %d;\n", detectorStruct.gain_20mm);
            myFile.printf("\"gain_30mm\" = %d;\n", detectorStruct.gain_30mm);
            myFile.printf("\"gain_40mm\" = %d;\n", detectorStruct.gain_40mm);
            myFile.printf("\"intTime_10mm\" = %d;\n", detectorStruct.intTime_10mm);
            myFile.printf("\"intTime_20mm\" = %d;\n", detectorStruct.intTime_20mm);
            myFile.printf("\"intTime_30mm\" = %d;\n", detectorStruct.intTime_30mm);
            myFile.printf("\"intTime_40mm\" = %d;\n", detectorStruct.intTime_40mm);
            myFile.printf("\"ir_10mm\" = %d;\n", irStruct.ir_10mm);
            myFile.printf("\"ir_20mm\" = %d;\n", irStruct.ir_20mm);
            myFile.printf("\"ir_30mm\" = %d;\n", irStruct.ir_30mm);
            myFile.printf("\"ir_40mm\" = %d;\n", irStruct.ir_40mm);
            myFile.printf("ledStatus: %d;\n", detectorStruct.LEDpattern);
            myFile.printf("\"sensor_10mm\" = %d;\n", detectorStruct.sensor_10mm);
            myFile.printf("\"sensor_20mm\" = %d;\n", detectorStruct.sensor_20mm);
            myFile.printf("\"sensor_30mm\" = %d;\n", detectorStruct.sensor_30mm);
            myFile.printf("\"sensor_40mm\" = %d;\n", detectorStruct.sensor_40mm);
            myFile.printf("\"state_of_charge\" = %f;\n", stateOfCharge);
            myFile.printf("\"temp_skin\" = %d;\n", infoStruct.temp_skin);
            myFile.printf("\"temp_amb\" = %d;\n", infoStruct.temp_amb);
            myFile.printf("time = %d;\n\n", infoStruct.time);
           // myFile.println(detectorStruct.sensor_10mm);
     
            // Close the file connection here
            myFile.close();
            // Success!
        } else {
           // Error code 6: Failed to open file
           sd_card_status = 6;
        }
   }
  }
  
  infoStruct.SDCardStatus = sd_card_status;

  // Send all structs in succession, eliminating the need for a second loop
  // It is important that we send these in this order because of the infoByte
  RFduinoBLE.send((char *)&infoStruct, sizeof(infoStruct));
  RFduinoBLE.send((char *)&detectorStruct, sizeof(detectorStruct));
  RFduinoBLE.send((char *)&irStruct, sizeof(irStruct));

  // Sync if sync is on
  if(shouldSync) {
    syncData();
  }
}

void RFduinoBLE_onReceive(char *data, int len) {
  // s is sync
  if(data[0] == 's') {
    shouldSync = true;
  }
  // 4 is write 
  else if(data[0] == '4') {
      sd_card_status = 4;
    } else {
      // 5 is writeable but not writing at the moment
      sd_card_status = 5;
      /*
       * Split into multiple files here to mimic a stop button
       * 
       */

        int i = 0;
        String tmpStr = "log_" ;
        while(SD.exists(wfilename)) {
          
          tmpStr = "log_" + (String) i;
          tmpStr += ".txt";
          /*
          File tmpFile = SD.open("errors.txt", FILE_WRITE);
          tmpFile.println(tmpStr);
          tmpFile.println("\n");
          tmpFile.close();
          */
          for(int j = 0; j < tmpStr.length(); j++) {
            wfilename[j] = tmpStr[j];
          }
          i += 1;
        }
        File trackerFile = SD.open("tracker.txt", FILE_WRITE);
        if(trackerFile) {
          // Add this new filename to the tracker file
          String toWrite = "filename = ";
          toWrite += tmpStr;
          toWrite += ";";
          //sd_card_status = 8;
          trackerFile.println(toWrite);
          trackerFile.close();
        }
       /*
        * Split functionality ends here
        */
    }

}

String extractData(String str) {

  
  //Serial.println("Received string: ");
  //Serial.println(str);
  String finalVal = "";
  bool foundEquals = false;
  for(int i = 0; i < str.length(); i++) {
    char tmp = str[i];
    if(tmp == '=' || tmp == ':') {
      foundEquals = true;
      continue;
    }
    if(foundEquals && tmp != ' ') {
      
      if(tmp == ';') {
        break;
      } 
      finalVal += str[i];
    }
  }
  return finalVal;
}

void RFduinoBLE_onConnect() {
  // Increment file counter to write to a new file on a new connection
  int i = 0;
        String tmpStr = "log_" ;
        while(SD.exists(wfilename)) {
          
          tmpStr = "log_" + (String) i;
          tmpStr += ".txt";
          for(int j = 0; j < tmpStr.length(); j++) {
            wfilename[j] = tmpStr[j];
          }
          i += 1;
          continue;
          File trackerFile = SD.open("tracker.txt", FILE_WRITE);
          if(trackerFile) {
            // Add this new filename to the tracker file
            String toWrite = "filename = ";
            toWrite += tmpStr;
            toWrite += ";";
          
            trackerFile.println(toWrite);
            //sd_card_status = 8;
            trackerFile.close();
          }
        }
         
}

void RFduinoBLE_onDisconnect() {
  // Turn off syncing
  shouldSync = false;
}

/*
 * Basic overview of how this method works:
 * This method looks for a tracker.txt file
 * and then uses the lines in that file to 
 * get a list of all the files it needs to sync
 * It then takes all the lines in the list, takes each line,
 * parses it, and then converts it to the appropriate data type
 * and then sends it to the device, after packing it in a struct
 * Sync codes are used to begin and terminate the sync operations
 * 32: Start Sync, 42: End Sync 
 * Do not delete the tracker.txt file, or change it, as that will affect 
 * syncing.
 */
void syncData() {
  
  // The tracker file keeps track of the the files we plan on sending
  // If the tracker file doesn't exist, we just send infoByte 58, which
  // means that there is no data on file to sync
  File trackerFile = SD.open("tracker.txt");
  sync_packet syncStatus;
  file_packet fileStruct;
  file_packet2 fileStruct2;
  
  if(trackerFile) {
    
    // Send 32
    // Start Sync
    syncStatus.infoByte = 32; 
    file_packet2 fileStruct2;
    fileStruct2.infoByte = 29;
     
    // Send this to the device
    RFduinoBLE.send((char *)&syncStatus, sizeof(syncStatus));
    String tline = "";
    while(trackerFile.available()) {
      int ta = trackerFile.read();
      int tb = trackerFile.peek();
      if((char)tb == ';') {
        tline += (char)ta;
        tline += ";";

       String filename = extractData(tline);
        
        // Convert this to a charArray so SD.open can open it
        // The prefix controls what directory to open
        //String prefix = "Logs/";
        String finalName = filename;
        char tempname [19] = "";
        finalName.toCharArray(tempname, sizeof(finalName));
        filename.toCharArray(fileStruct.fileNamed, sizeof(filename));
        filename.toCharArray(fileStruct2.fileNamed, sizeof(filename));
        Serial.println("Now opening file");
        //Serial.println(tempname);
        
        // Define the structs we plan on using here
        detector_packet detectorStruct;
        info_packet infoStruct;
        ir_packet irStruct;
        
        infoStruct.SDCardStatus = 0;
        infoStruct.infoByte = 0;
        detectorStruct.infoByte = 1;
        irStruct.infoByte = 2;
        fileStruct.infoByte = 6;

        
        // open the file. note that only one file can be open at a time,
       // so you have to close this one before opening another.
       
       //char tempname [] = "Logs/log_1.txt";
       File dataFile = SD.open(tempname);

      /*
       * Data is logged to, and thus read from the SD card in the following order
       * Maps to the numbers on the left
       * 0: "cell_voltage"
       * 1: "gain_10mm"
       * 2: "gain_20mm"
       * 3: "gain_30mm"
       * 4: "gain_40mm"
       * 5: "intTime_10mm"
       * 6: "intTime_20mm"
       * 7: "intTime_30mm"
       * 8: "intTime_40mm"
       * 9: "ir_10mm"
       * 10: "ir_20mm"
       * 11: "ir_30mm"
       * 12: "ir_40mm"
       * 13: ledStatus
       * 14: "sensor_10mm"
       * 15: "sensor_20mm"
       * 16: "sensor_30mm"
       * 17: "sensor_40mm"
       * 18: "state_of_charge"
       * 19: "temp_skin"
       * 20: "temp_amb"
       * 21: time
       */
      // if the file is available, read from it:
      if (dataFile) {
        int counter = -1;
        String line = "";
        while (dataFile.available()) {
          int a = dataFile.read();
          int b = dataFile.peek();
          if((char)b == ';') {
            line += (char)a;
            line += ';';
            counter++;
            String extVal = extractData(line);

            if(counter == 0) {
          
               // Cell Voltage  
              // Convert to float
              char man [] = "";
              extVal.toCharArray(man, sizeof(extVal));
              float cell_voltage = atof(man);
              infoStruct.cellVoltage = cell_voltage;
          
            } else if(counter == 1) {
          
              // gain_10mm
             // Convert to byte
             detectorStruct.gain_10mm = extVal.toInt();
          
            } else if(counter == 2) {

              // gain_20mm
             // Convert to byte
             detectorStruct.gain_20mm = extVal.toInt();
          
           } else if(counter == 3) {

             // gain_30mm
            // Convert to byte
            detectorStruct.gain_30mm = extVal.toInt();
          
          } else if(counter == 4) {

            // gain_40mm
            // Convert to byte
            detectorStruct.gain_40mm = extVal.toInt();
          
          } else if(counter == 5) {

            // intTime_10mm
            // Convert to byte
            detectorStruct.intTime_10mm = extVal.toInt();
          
          } else if(counter == 6) {

            // intTime_20mm
            // Convert to byte
            detectorStruct.intTime_20mm = extVal.toInt();
         
          } else if(counter == 7) {

            // intTime_30mm
            // Convert to byte
            detectorStruct.intTime_30mm = extVal.toInt();
         
          } else if(counter == 8) {

            // intTime_40mm
            // Convert to byte
            detectorStruct.intTime_40mm = extVal.toInt();
         
          } else if(counter == 9) {

            // ir_10mm
            // Convert to byte
            irStruct.ir_10mm = extVal.toInt();
         
          } else if(counter == 10) {
          
            // ir_20mm
            // Convert to byte
            irStruct.ir_20mm = extVal.toInt();
       
          } else if(counter == 11) {
          
            // ir_30mm
            // Convert to byte
            irStruct.ir_30mm = extVal.toInt();
       
          } else if(counter == 12) {
        
            // ir_40mm
            // Convert to byte
            irStruct.ir_40mm = extVal.toInt();
       
          } else if(counter == 13) {
          
            // ledStatus
            // Convert to byte
            detectorStruct.LEDpattern = extVal.toInt();
       
          } else if(counter == 14) {

            // sensor_10mm
            // Convert to short
            detectorStruct.sensor_10mm = extVal.toInt();
          
          } else if(counter == 15) {

            // sensor_20mm
            // Convert to short
            detectorStruct.sensor_20mm = extVal.toInt();
          
          } else if(counter == 16) {

            // sensor_30mm
            // Convert to short
            detectorStruct.sensor_30mm = extVal.toInt();
          
          } else if(counter == 17) {

            // sensor_40mm
            // Convert to short
            detectorStruct.sensor_40mm = extVal.toInt();
          
          } else if(counter == 18) {

            // State of Charge
            // Convert to float
            char man [] = "";
            extVal.toCharArray(man, sizeof(extVal));
            float state_of_charge = atof(man);
            infoStruct.stateOfCharge = state_of_charge;
          
          } else if(counter == 19) {

            // Skin temperature
            // Convert to short
            infoStruct.temp_skin = extVal.toInt();
          
          } else if(counter == 20) {

            // Amb. temperature
            // Convert to short
            infoStruct.temp_amb = extVal.toInt();
          
          } else if(counter == 21) {
             // Reset counter here
             counter = -1;
            // Time
            // Convert to Int
            infoStruct.time = extVal.toInt();

           // Send filename here
           //fileStruct.fileNamed = tempname;
           RFduinoBLE.send((char *)&fileStruct, sizeof(fileStruct));
          // Send structs here
          RFduinoBLE.send((char *)&infoStruct, sizeof(infoStruct));
          RFduinoBLE.send((char *)&detectorStruct, sizeof(detectorStruct));
          RFduinoBLE.send((char *)&irStruct, sizeof(irStruct));
          
        }
        line = "";
      } else {
        
        line += (char)a;
      }
      //counter++; 
    }

    // Send end signal here
    RFduinoBLE.send((char *)&fileStruct2, sizeof(fileStruct2));
    dataFile.close();  
    Serial.println("File connection closed");
    
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening log_0.txt");
  }     

        // At the very end of all processing, we clear the tline
        tline = "";
      } else {

        tline += (char)ta;
      }
    }
    trackerFile.close();
    // Turn sync off
    shouldSync = false;
    // Stop sync
    syncStatus.infoByte = 42;
    // Send this to the device
    RFduinoBLE.send((char *)&syncStatus, sizeof(syncStatus));
  } else {
    // Nothing to sync
    syncStatus.infoByte = 58;
    // Send this using BLE
    RFduinoBLE.send((char *)&syncStatus, sizeof(syncStatus));
  }
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



