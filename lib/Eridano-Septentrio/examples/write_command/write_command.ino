//Allows for writing any command to the GNSS through the Serial monitor
//The GNSS' response is printed in the serial monitor as well
//Be aware that the chosen baudrate must match, with some libraries not all baudrates are possible
//
//This code was tested with an Arduino with only one serial port and used AltSoftSerial to remedy:
//If you have more than 1 serial port, you do not need this library 
//
//author: Chiara de Saint Giniez

#include <AltSoftSerial.h>
#include <Septentrio_Arduino_driver.h>
#include <septentrio_structs.h>

//#define txPin 9
//#define rxPin 8
#define baudrate 115200

AltSoftSerial mySerial;

void setup() {
  Serial.begin(baudrate);
  mySerial.begin(baudrate);

  while(!Serial);
  Serial.println("Ports initiated");
  mySerial.write("grc\n");
  if (mySerial.available()==0) //check that port accepts command, force it otherwise (see septentrio doc in CLI)
  {
    mySerial.write("SSSSSSSSSS\n");
  }
  Serial.println("Input connection was initially forced");
  mySerial.write("setDataInOut, COM1, CMD");
}

void loop() {
  while(Serial.available()>0)
  {
    mySerial.write(Serial.read());
  }
  
  //read the response from gnss for user readability
  while (mySerial.available()>0)
  {      
    Serial.print((char)mySerial.read());
  }
}
