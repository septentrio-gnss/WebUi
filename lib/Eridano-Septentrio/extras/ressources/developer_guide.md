# <center> Developer guide 
---
## Content of the library

Two classes are defined in this library:
* **SEPTENTRIO_GNSS** for general input and output limited to the receiver. 
* **SEPTENTRIO_NTRIP** for all operations related to NTRIP (see the NTRIP Client example's folder for more explanations). A caster and port must be provided when initializing. It can function with NTRIP 1 and 2, and HTTP protocol.
Both need a **two-way serial connection** to the receiver to be initialized and work correctly. 

Different objects are defined in a separate file *septentrio_structs.h*:
* **sbfBuffer_t** contains all that is needed to store and process an SBF message
* **nmeaBuffer_t** contains all that is needed to store and process an NMEA message
* **tempBuffer_t** is a buffer for the header part of SBF messages (see [documentation](TODO) for what it entails)
* **ntripTempBuffer_t** is a buffer for the header part of incoming NTRIP response, compatible with NTRIP v1 and v2
* **ntripBuffer_t** is a simple buffer to store NTRIP data
* **ntripProperties_t** is a structure to store the data concerning the NTRIP caster 

## SEPTENTRIO_GNSS class

This class contains:
* the different buffer for SBF and NMEA 
* the necessary functions to setup the receiver
* the conversion functions for all types defined in [SBF documentation](TODO)
* the functions to test the integrity and validity of messages
* the functions for processing the data and buffers
* the functions to enable and disable debug mode

Here is how messages are processed in checkNewByte:
<center> SBF version </center>
<img align=center src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/SBF_byteFlow.jpg"/>
<center> NMEA version </center>
<img align=center src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/NMEA_byteFlow.jpg"/>

## NTRIP connection
#### How it works
This example is composed of two files:
* .ino : containing the program sent to the Arduino
* secrets.h : containing the different data needed for the connection (at the very minimum, the WiFi IP and password) which can be freely modified by the user.  
This example creates an NTRIP client connected to WiFi provided by the user and connects to an NTRIP caster, sending authentication, NMEA and/or another custom field if given by the user.  
Once connected, the program will process data depending on their type:
* gnss/data will be passed directly to the receiver for correction (in chunks if it is how the messages are being sent)
* sourcetable will be used to verify and potentially correct the known NTRIP properties
* other type of data will be ignored 

This program can also be run with a debug mode to use with a monitor (ex. the Serial Monitor) which will display : 
* A successful connection with the corresponding code **200**
* A failed connection will display the error type as well as the header(s) corresponding
The most common NTRIP errors have special instruction but otherwise a list of the error codes can be found online [here](https://receiverhelp.trimble.com/r750-gnss/ioConfig.html)

## NTRIP connection

The NTRIP example is the most intricate of the library. You can take a look at the [user manual](https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/ressources/user_manual.md) for a general explanation of NTRIP. This part aims to explain the workflow of the example, starting with a general overview:<br>
<img align=center src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/ntrip_general_workflow_portrait.jpg"/> 
This example uses the HTTP protocol to connect to the NTRIP caster, the user first has to provide different inputs to fill the request message (caster, mountpoint, id and password). Connection is done thanks to the WiFi library of Arduino and needs to be adapted to each boards. Once connection with WiFi and caster has been established, the request buffer is sent to the caster.\
Once the response has been received, the program first checks for a success code ('200'); if it finds it, it then checks for NTRIP version through the syntax of the 200 response and extract the relevant data from the header; otherwise, unless debug is enabled and the error code is printed, no further process is done.\
If the connexion is successful, the program then checks for data type and will only process sourcetables, by comparing requirements for the mountpoint, or correction data, by passing them to the receiver.
When data stops being sent, the program checks that the connexion still exist (and re-establish it otherwise) and sends the request again if the timeout limit has passed. If it still doesn't work, the program will try double-checking mountpoint requirements and adjusting if possible. \
Below are more detailed flowchart of 
<br clear="left"/>
<p>
<img align=center src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/ntrip_header_processing_portrait.jpg" width="38%" height="38%"/>
<img align=center src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/ntrip_content_processing_portrait.jpg" width="60%" height="60%"/>
</p>

