//Parsing example of a NMEA message : GNGGA
//This code only displays the most recent data, it does not keep it in memory.
//The message Id must provide all five or more characters
//If you do not wish to specify the talker ID, you can use the character '*'
//
//This code was tested with an Arduino with only one serial port and used AltSoftSerial to remedy:
//If you have more than 1 serial port, you do not need this library 
//
//author: Chiara de Saint Giniez

#include <AltSoftSerial.h>
#include <Septentrio_Arduino_driver.h>
#include <septentrio_structs.h>

#define baudrate 115200
#define msgId "GNGGA"

AltSoftSerial mySerial;
SEPTENTRIO_GNSS myGNSS;

void setup() {
  Serial.begin(baudrate);
  mySerial.begin(baudrate);
  Serial.println("Connections initiated");
  if (myGNSS.begin(mySerial)==false) //The serial port need to be able to send and receive data to work properly
  {
    Serial.println("Failed to connect to GNSS");
  }
  Serial.println("Connected to GNSS");
}

void loop() {

  while (mySerial.available()>0)
  {
    myGNSS.checkNewByte(myGNSS.tempBuffer, mySerial.read(), msgId);
  }

  if (Serial && myGNSS.NMEABuffer->alreadyRead==0)
  {
    if (myGNSS.NMEABuffer->correctChecksum)
    {
      Serial.println("Incoming message :");
      for (int i=0;i<myGNSS.NMEABuffer->msgSize;i++)
      {
        Serial.print(myGNSS.NMEABuffer->data[i], DEC);
      }
      Serial.println("");
    }
    else
    {
      Serial.println("Incorrect checksum, make sure there is no connection issue.");
    }
  }
  delay(250);
}
