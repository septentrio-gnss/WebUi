#include <RingBuffer.h>
#include <SoftwareSerial.h>

#include <Septentrio_Arduino_driver.h>
#include <septentrio_structs.h>
#include <arduino_base64.hpp>
#include "secrets.h"

#include "WiFiS3.h"
#define USER_AGENT "ntrip_prog/1.0"
const char casterHost[] = "caster.world";
const uint16_t casterPort = 2101;
const char casterUser[]= "USER01";
const char casterUserPwd[]= "PWD";
const char mountPoint[]= "MountPoint";

#define baudrate 9600
#define rtcmTimeOut 5000 //in ms
#define CLIENT_BUFFER_SIZE 500 //should have to be changed
#define RTCM_MAX_SIZE 512*4 //TODO: copied from sparkfun
#define NMEA_MAX_SIZE 100 //82 is standard but allows more for custom messages ->put in c++
#define AUTH_MAX_SIZE 100 //TODO ->put in c++
#define replyTimeout 5000 //for the first connection

const byte rxPin = 13;
const byte txPin = 12;

//WiFi stuff
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key index number (needed only for WEP)
int status = WL_IDLE_STATUS;

SEPTENTRIO_NTRIP myNTRIP;
WiFiClient ntripClient;
SoftwareSerial mySerial(rxPin, txPin);

unsigned long start=millis();
int index=0;

void setup() {
  Serial.begin(baudrate);
  mySerial.begin(baudrate);
  while (!Serial){;} //comment if you use the Serial monitor, you need to comment all the Serial.print in the code also

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(3000); // wait for 3 seconds so as not to pound
  }
  Serial.println("Connected to WiFi");
  myNTRIP.enableDebug(Serial);

  if (casterUser!="")
  {
    myNTRIP.ntripProperties->userCredent64 = new char[AUTH_MAX_SIZE];
    myNTRIP.ntripProperties->userCredent64= new char[AUTH_MAX_SIZE];
    myNTRIP.ntripProperties->userCredent64[0]='\0'; // '/0' used as marker of empty, you can also just comment this line and the previous one
  }
  myNTRIP.ntripProperties->caster.Host = casterHost;
  myNTRIP.ntripProperties->caster.Mountpoint=mountPoint;
  myNTRIP.ntripTempBuffer = new ntripTempBuffer_t;
  myNTRIP.ntripTempBuffer->data = new uint8_t[ntripHeaderMaxSize];
  myNTRIP.ntripProperties->nmeaData = new char[nmeaMaxSize];
  int ntripRequestMaxSize=requestStandardMaxSize+ (myNTRIP.ntripProperties->userCredent64!=nullptr)*RequestAuthMaxSize+ (myNTRIP.ntripProperties->nmeaData!=nullptr)*nmeaMaxSize+ (myNTRIP.ntripProperties->custom!=nullptr)*RequestCustomMaxSize;
  myNTRIP.ntripRequestBuffer = new char[ntripRequestMaxSize];
  //you can put a default nmea here
  //strncpy(myNTRIP.ntripProperties->nmeaData, "$GPGGA,time,lat,N,long,E,1,12,1.0,0.0,M,0.0,M,,*FF", nmeaMaxSize);
}

void loop() {
  if (!myNTRIP.ntripProperties->connected)
  { 
    myNTRIP.ntripProperties->connected = setupNtripClient((millis()-start)<(2*rtcmTimeOut)); //initialize connection message
  }
  if (Serial)
  {
    if (!ntripClient.connected())
    {
      ntripClient.connect(casterHost, casterPort); //sends request for gnss
    }
    if ((millis()-start)>rtcmTimeOut)
    {
      ntripClient.write(myNTRIP.ntripRequestBuffer, strlen(myNTRIP.ntripRequestBuffer));
      delay(100);
    } 
    if (myNTRIP.ntripProperties->transferEncoding) //for chunks
    {
      while (ntripClient.available()>0)
      {
        int chunkSize=getChunkSize();
        Serial.print("Chunk : ");
        Serial.println(chunkSize);

        if (chunkSize!=0)
        {
          for (int i=0; i<chunkSize;i++)
          {
            mySerial.write(ntripClient.read());
          }
          ntripClient.read(); //clear '\r'
          ntripClient.read();  //clear '\n'
          start=millis();
        }
      }
    }
    else 
    {
      while (ntripClient.available()>0)
      {
        mySerial.write(ntripClient.read());
      }
    }
    if (Serial) {Serial.println("End of transmission");}
    ntripClient.flush();
    while (ntripClient.available()){ntripClient.read();}
  }
  delay(100);
}

bool setupNtripClient(bool useMountPoint)
{
  bool connectionSuccess=false;
  if (!myNTRIP.ntripProperties->connected)
  {
    if (!ntripClient.connect(casterHost, casterPort))
    {
      Serial.println(F("Failed to connect to caster, will retry"));
      //DIGITAL_WRITE(LED_BUILTIN, LOW);
    }
    else
    {
      Serial.print(F("Connected through "));
      Serial.print(casterHost);
      Serial.print(F(":"));
      Serial.println(casterPort);
      Serial.print(F("Will use the mountpoint : "));
      Serial.println(useMountPoint ? mountPoint : "");
      if (myNTRIP.ntripProperties->userCredent64!=nullptr)
      {
        Serial.print(F("Using credentials : "));
        Serial.print(casterUser);
        Serial.print(F(" and "));
        Serial.println(casterUserPwd);

        //create credential string and fill it
        uint8_t userCredentials[sizeof(casterUser)+sizeof(casterUserPwd)-1];
        snprintf((char*)userCredentials, sizeof(casterUser)+1+sizeof(casterUserPwd), "%s:%s", casterUser, casterUserPwd);
        //encode through base64
        base64::encode(userCredentials, sizeof(userCredentials), myNTRIP.ntripProperties->userCredent64);
      }

      myNTRIP.createRequestMsg(USER_AGENT, useMountPoint);
      Serial.println("Request message: ");
      Serial.println(myNTRIP.ntripRequestBuffer);

      //sending the request
      while (ntripClient.available()>0) {ntripClient.read();}
      ntripClient.write(myNTRIP.ntripRequestBuffer, strlen(myNTRIP.ntripRequestBuffer));

      //wait for reply
      unsigned long start=millis();
      while (!ntripClient.available())
      {
        if (millis() - start > replyTimeout)
        {
          Serial.println(F("Caster timed-out"));
          return false;
        }
      }
      start=millis();
      while (ntripClient.available()>0 && !connectionSuccess && (millis()-start)<replyTimeout)
      {
        connectionSuccess=myNTRIP.processAnswer(myNTRIP.ntripTempBuffer, ntripClient.read());
      }
      //failed to connect
      if (!connectionSuccess)
      {
        if (Serial)
        {
          Serial.println(F("Failed to connect to caster"));
        }
      }
      else 
      {
        Serial.println(ntripClient.available());
        if (ntripClient.available()==0)
        {
          Serial.println("Failed !");
          connectionSuccess=false;
          myNTRIP.createRequestMsg(USER_AGENT, false);
          ntripClient.write(myNTRIP.ntripRequestBuffer, strlen(myNTRIP.ntripRequestBuffer));
          delay(100);
          start=millis();
          while (ntripClient.available()>0 && !connectionSuccess && (millis()-start)<replyTimeout)
          {
            connectionSuccess=myNTRIP.processAnswer(myNTRIP.ntripTempBuffer, ntripClient.read());
          }
          if (myNTRIP.ntripProperties->nmeaData!=nullptr)
          {
            delete[] myNTRIP.ntripRequestBuffer;
            int ntripRequestMaxSize=requestStandardMaxSize+ (myNTRIP.ntripProperties->userCredent64!=nullptr)*RequestAuthMaxSize+ (myNTRIP.ntripProperties->nmeaData!=nullptr)*nmeaMaxSize+ (myNTRIP.ntripProperties->custom!=nullptr)*RequestCustomMaxSize;
            myNTRIP.ntripRequestBuffer = new char[ntripRequestMaxSize];
          }
          connectionSuccess=false;
        }
        //DIGITAL_WRITE(LED_BUILTIN, HIGH);
        else if (Serial)
        {
            Serial.println(F("Successfully connected to caster"));         
        }
      }
    }
  }
  else 
  {
    connectionSuccess=true;
    if (Serial)
    {
      Serial.println(F("Already connected to NTRIP"));
    }
  } 
  return connectionSuccess;
}

int getChunkSize()
{
  int chunksize=0;
  bool sizeInProgress=true;
  while (sizeInProgress)
  {
    uint8_t byte = ntripClient.read();
    if(byte >= '0' && byte <= '9') 
    {
      chunksize = chunksize*16+byte-'0';
    }
    else if(byte >= 'a' && byte <= 'f')
    {
      chunksize = chunksize*16+byte-'a'+10;
    }
    else if(byte >= 'A' && byte <= 'F')
    {
      chunksize = chunksize*16+byte-'A'+10;
    }
    else if(byte == '\r')
    {
      if (ntripClient.read()!='\n')
      {
        chunksize=0;
      }
      sizeInProgress=false;
    }
    else if (byte == ';')
    {
      while (ntripClient.read()!='\r'){;}
      if (ntripClient.read()!='\n')
      {
        chunksize=0;
      }
      sizeInProgress=false;
    }
    else
    {
      chunksize=0;
      sizeInProgress=false;
    }
  }
  return chunksize;

}