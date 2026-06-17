//Logging example of a Septentrio SBF block : PVTGeodetic
//This code was not tested on hardware

#include <Septentrio_Arduino_driver.h>
#include <septentrio_structs.h>

#include <SD.h>
#include <AltSoftSerial.h>

#include <Septentrio_Arduino_driver.h>
#include <septentrio_structs.h>

//#define rxPin 8
//#define txPin 9
#define baudrate 9600

AltSoftSerial mySerial;
SEPTENTRIO_GNSS myGNSS;
bool foundSentence = false;

File myFile;
bool correctMsg=false;
bool fileExist=false;

void setup() {
  Serial.begin(baudrate);
  mySerial.begin(baudrate);

  while (!Serial.available()){;}

  if (!myGNSS.begin(mySerial, 5000))
  {
    Serial.println("No connection");
    while(1);
  }
  Serial.println("GNSS connected initialized");

  if (!SD.begin())
  {
    Serial.println("SD card not found or not accessible...");
    while(1);
  }
  Serial.println("SD card initialized");

  delay(100); //wait for multiple characters
  while (Serial.available()>0) {Serial.read();}

  if (!SD.exists("../DATA"))
  {
    SD.mkdir("../DATA");
  }
  myFile = SD.open("../DATA/PVTGEodetic2_log.txt", FILE_WRITE);

  if(!myFile)
  {
    Serial.println("Failed to create the document...");
    while(1);
  }
  else 
  {
    fileExist=true;
  }

}

void loop() {
  if (Serial.available())
  {
    myFile.close();
    fileExist=false;
    Serial.println("Logging ended");
  }
  else 
  {
    foundSentence=false;
    while (mySerial.available()>0)
    {
      foundSentence = myGNSS.checkNewByte(myGNSS.tempBuffer, mySerial.read(), SBF_SENTENCE, 4007); 
    if (fileExist && foundSentence)
    {
      correctMsg=myGNSS.isSBFValid(myGNSS.SBFBuffer, 4007);
      myFile.write(myGNSS.SBFBuffer->data,myGNSS.SBFBuffer->msgSize);
    }
  }
  


}
