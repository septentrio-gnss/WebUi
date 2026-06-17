<div align="center">

# Eridano: Septentrio Arduino Library

<img align=center src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/Logo.png" width=225 height=225>

This Arduino library has been created to make Septentrio GNSS easier to use with Arduino hardware. <br>
**Make sure that both the Arduino's and the receiver's baudrate match to use this library (keep in mind that the default baudrate for Septentrio is 115200 but it is not supported by AltSoftSerial used in some examples).**  <br> 

<br>

## Authors 
|Name                   | GitHub   |  
|-----------------------|----------|
| chiara de Saint Giniez| [chiara-septentrio](https://github.com/chiara-septentrio)|
| Ariel Kriss Sany| [sariel01-septentrio](https://github.com/sariel01)|

<br>

## Maintainer 
| GitHub |
|--------|
| <a href="https://github.com/septentrio-users">septentrio-users</a> </br> |  

<br>

## DO YOU HAVE ANY QUESTIONS?

## Septentrio resources
This library is not officially supported by Septentrio however Septentrio can provide great help around Septentrio GNSS receivers
| Septentrio website                                    | Contact                                                          | Resources |
| :-:                                                   | :-:                                                              | :-: |
| [Septentrio home page](https://web.septentrio.com/GL-SSN-home) | [Septentrio contact page](https://web.septentrio.com/GH-SSN-contact) | [Septentrio resource page](https://web.septentrio.com/GH-SSN-support) |

</div>

### Table of contents
* [What is Eridano](#what-is-eridano)
* [Installation](#installation)
* [How to use guides](#how-to-use-guides)
* [Credits](#credits)

## What is Eridano
Eridano is an Arduino Library for Septentrio GNSS receivers, it allows communication between Arduino products and Septentrio receivers. By using its examples or creating your own, you can use all sorts of features from Septentrio receivers with the major ones being: 
* Parsing of SBF or NMEA
* Logging
* Corrections via NTRIP  

[Septentrio](https://www.septentrio.com) is a leading provider in high-level GNSS solution known for providing reliable and secure positioning technology all around the globe.

## Installation
Eridano has been developed with the [official Arduino IDE](https://www.arduino.cc/en/software). Once it has been installed, the library can be easily implemented through it by:
* A: Importing the library via the IDE (only available after approval from community)
* B: Downloading the zip and importing it in the IDE (we would recommend putting it in your /library folder)
You can then upload any of the examples through the IDE to the board.
<br>

| A: Including the library                                    | B: Using ZIP file                                                       |
|-----------------------|----------|
| <img align=left src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/Include%20library.png" size="20%" > | <img align=right src="https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/images/Zip%20library.PNG" SIZE="20%"> |
<br>

## How to use guides
This open source projects provides user with two guides:
* one for users describing all functionalities
* one for developers, describing how the library is structured

The user manual aims to explain how to use the examples and adapt them to the user's need.  
<div align="center"> 

| [Go to User Manual](https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/ressources/user_manual.md) |
|---|

 </div>
The developer manual aims to explain the logic behind the library for those who want to implement more features or keep the current ones up to date.  
<div align="center"> 

| [Go to Developer Guide](https://github.com/septentrio-gnss/Septentrio_Arduino_library/blob/main/extras/ressources/developer_guide.md) |
|---| 

</div>

## <center> Credits </center>
I would like to thank Nathan Seith and the Sparkfun team for allowing to use their [library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library/) for their u-blox modules as a inspiration for this library.
