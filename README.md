# SeptenUI

**SeptenUI** is a universal, web-based interface designed to monitor, configure, and control **Septentrio mosaic-g5** GNSS receivers. 

This project runs on an **ESP32-S3** microcontroller, acting as a bridge between the high-precision GNSS module and the user. It removes the need for proprietary desktop software for field operations, offering a responsive HTML5 dashboard accessible from any browser (PC, Mobile, Tablet).

---

##  Table of Contents
- [Project Overview](#project-overview)
- [Key Features](#key-features)
- [Hardware Architecture](#hardware-architecture)
- [Software Prerequisites](#software-prerequisites)
- [Installation Guide](#installation-guide)
- [User Guide](#user-guide)
- [Trade-Off Analysis](#trade-off-analysis)
- [Technical Implementation](#technical-implementation)
- [Troubleshooting](#troubleshooting)

---

##  Project Overview

Developed as an internship project at Septentrio, **SeptenUI** demonstrates the capabilities of the **mosaic-g5** module when paired with modern IoT hardware. 

The system operates as a standalone web server. It parses binary SBF (Septentrio Binary Format) data in real-time to display precise positioning metrics, while simultaneously handling NTRIP corrections and Bluetooth relaying.

---

##  Key Features

* **Real-Time Dashboard:**
    * Visualization of Position (LLA), Velocity (NED), and Attitude (Roll/Pitch/Heading).
    * **Dynamic Skyplot:** Real-time rendering of satellites (GPS, GLONASS, Galileo, BeiDou) with Signal-to-Noise (C/N0) and usage status.
    * **Quality Indicators:** Visual bars for RF Spectrum, CPU Load, and Signal Integrity parsed from SBF `QualityInd` blocks.
* **Connectivity:**
    * **Internal NTRIP Client:** The ESP32 connects to a caster via Wi-Fi to inject RTCM corrections into the receiver for RTK Fixed accuracy.
    * **Bluetooth Relay (BLE):** Broadcasts NMEA data to mobile apps using standard Nordic UART Service.
* **Data Logging:** Remote triggering of logging sessions directly to the mosaic-g5's on-board SD card (supports both SBF and NMEA formats).
* **Expert Console:** A raw terminal to send proprietary commands  and view filtered receiver responses.
* **Cross-Platform Design:** Fully responsive interface optimized for smartphones (GSM) and tablets. The navigation menu becomes scrollable and data grids automatically stack vertically on smaller screens, enabling full field control without a laptop.

---

##  Hardware Architecture

### Required Components
1.  **Microcontroller:** ESP32-S3 Development Board (must support native USB/BLE).
2.  **GNSS Receiver:** Septentrio mosaic-g5 evaluation board (or integration kit).
3.  **Connection:** UART (Serial) interface.

### Wiring Diagram
The communication relies on **Serial2** on the ESP32-S3.

| ESP32-S3 Pin | mosaic-g5 Pin | Description |
| :--- | :--- | :--- |
| **GND** | **GND** | Common Ground reference |
| **GPIO 21 (RX)** | **COMx TX** | Data Reception (ESP32 reads SBF/NMEA) |
| **GPIO 12 (TX)** | **COMx RX** | Command Transmission & RTCM Injection |

> **Note:** Ensure the logic voltage levels (3.3V) are compatible.

---

##  Software Prerequisites

To build and flash this project, you need:

1.  **IDE:** [VS Code](https://code.visualstudio.com/) with **PlatformIO** (Recommended) OR Arduino IDE 2.x.
2.  **ESP32 Board Package:** Version 2.0.11 or higher.

### Dependencies (Libraries)
The following libraries must be installed:
* `ESPAsyncWebServer` & `AsyncTCP`: For the HTML/JS interface.
* `WebSockets` (by Markus Sattler): For low-latency telemetry (10Hz).
* `ArduinoJson` (v7.x): For JSON serialization.
* `ESP32 BLE Arduino`: For Bluetooth functionality.
* `mbedtls`: For Base64 encoding (NTRIP authentication).

---

##  Installation Guide

### 1. Filesystem Upload (SPIFFS)
The web interface files (`index.html`, `style.css`, `chart.js`) are stored in the ESP32's flash memory.
1.  Navigate to the project data folder.
2.  **PlatformIO:** Run the task `Platform` -> `Upload Filesystem Image`.
3.  **Arduino IDE:** Use the "ESP32 Sketch Data Upload" tool.

### 2. Firmware Flashing
1.  Open `main.cpp`.
2.  Select your board (e.g., `ESP32S3 Dev Module`).
3.  Compile and Upload.

### 3. First-Time Configuration
1.  On the first boot, the system creates a Wi-Fi Access Point named **`DUALY_CONFIG`**.
2.  Connect to this network (Password is open or check source).
3.  Navigate to `http://192.168.3.1` in your browser.
4.  Go to the **Configuration** tab and enter your local Wi-Fi credentials.
5.  The system will reboot and connect to your local network.

---

##  User Guide

This section provides a detailed breakdown of every interface component, explaining the data sources and control mechanisms.

### 1. Dashboard View
The Dashboard is the mission control center of SeptenUI. It aggregates real-time data parsed from the mosaic-g5's binary SBF stream (10Hz update rate).

#### **A. Status Bar (Top)**
Located at the top of the interface, this bar provides instant system health feedback:
* **RTCM Stream:**  **Lights up** when RTCM correction data is successfully received via NTRIP or Bluetooth and injected into the receiver.
* **System Status:** Indicates the global health of the receiver. Red indicates a critical error reported by the `ReceiverStatus` SBF block.
* **Logging:** Turns **Green** when an active logging session (SBF or NMEA) is recording to the G5's SD card.
* **PVT Mode:** Displays the current solution type (e.g., `RTK FIXED`, `StandAlone`, `NO FIX`). The text color changes dynamically (Green for RTK Fixed, Orange for StandAlone).
* **Jamming:** Monitors the RF spectrum. Turns **Red** if the `RFStatus` block detects interference or spoofing attempts in the GNSS bands.
* **Connectivity:** Icons for **WiFi** and **Bluetooth** indicate the active state of the ESP32's wireless modules. They **light up** when they are activated.

#### **B. Position & Velocity Panel**
Displays the fundamental navigation solution derived from the `PVTGeodetic` and `PosCovGeodetic` SBF blocks:
* **Coordinates:** Latitude, Longitude (DMS format), and Ellipsoidal Height (meters).
* **Velocity:** North, East, and Up vectors in m/s.
* **Accuracy (Std Dev):** Next to each value, the Standard Deviation ($\sigma$) is displayed (e.g., `±0.015m`). This value represents the confidence level of the position/velocity calculation, extracted from the covariance matrices.

#### **C. Attitude Panel**
For dual-antenna setups or moving baselines, this panel visualizes orientation data from the `AttEuler` block:
* **Heading:** The direction the vehicle is pointing (0-360°).
* **Pitch & Roll:** Tilt angles in degrees.
* **Std Dev:** Accuracy estimates for angular measurements are displayed to validate the heading reliability.

#### **D. Quality Indicators**
A set of 7 bar charts providing a deep dive into the receiver's internal metrics (scaled 0-10), parsed from the `QualityInd` SBF block:
1.  **Overall:** Aggregate quality score.
2.  **Main RF:** Signal strength on the primary antenna input.
3.  **Aux1 RF:** Signal strength on the secondary antenna input.
4.  **Main Sig:** Quality of tracking signals on the main path.
5.  **Aux1 Sig:** Quality of tracking signals on the auxiliary path.
6.  **CPU:** Processing load of the mosaic-g5 core.
7.  **Base Meas:** Quality of the received base station measurements (RTCM).

#### **E. Dynamic Skyplot**
A visual representation of the GNSS constellation:
* **Visualization:** Plots satellites based on Azimuth (angle) and Elevation (distance from center).
* **Data Source:** Combines `SatVisibility` (positions) and `MeasEpoch` (Signal-to-Noise Ratio) blocks.
* **Filters:** Use the checkboxes below the chart to hide/show specific constellations (GPS, GLONASS, Galileo, BeiDou, SBAS, QZSS) to analyze coverage.


> **Startup Note:** Similar to Septentrio's RxControl software, the data stream parsing may take a few seconds to stabilize after a cold boot. 
> * If the dashboard remains empty after 15 seconds, **refresh the webpage**.
> * Alternatively, press the **RESET button** on the ESP32 to force a clean re-initialization of the serial buffers.
---

### 2. Expert Console
The Expert Console provides direct, low-level access to the receiver's command interface (SDS).

* **Command Input:** Type any standard Septentrio command (e.g., `lif,error`, `gdio`,  `lif,permisisons`) and press Enter. The command is forwarded via UART to the receiver.
* **Filtered Output:** To prevent the console from being flooded by high-speed NMEA streams, the display automatically filters out lines starting with `$G` (standard NMEA), showing only command responses and confirmation messages.
* **Preset Buttons:** Quick-access buttons for common tasks:
    * **Enable SBF Streams:** Configures the receiver to output the specific binary blocks required by the Dashboard (Stream1 on COM2).
    * **Save Config:** Sends (Save Configuration) to make settings persistent.
    * **Reboot:** Sends `reset` to soft-reboot the mosaic-g5 module.

---

### 3. Logging Control
**SeptenUI** allows you to manage data recording remotely.
* **Storage Location:** IMPORTANT - Data is logged directly to the **mosaic-g5's internal SD Card**, ensuring high-speed write performance without network bottlenecks.
* **SBF Logging:**
    * **Filter:** Specify which blocks to log (e.g., `PVTGeodetic,MeasEpoch`). Leave empty to log all.
    * **Action:** Click "Start SBF Logging" to create a file on the G5. The "Logging" LED in the status bar will turn green.
* **NMEA Logging:**
    * **Filter:** Specify sentences (e.g., `GGA,GSV`).
    * **Action:** Click "Start NMEA Logging" to begin recording ASCII data.

---

### 4. Configuration
Configure the connectivity and operating modes of the system.
> **Important Note on RTCM Sources:** > The system cannot receive RTCM corrections from both the internal Wi-Fi NTRIP client and the Bluetooth Relay simultaneously. 
> * **Enabling Bluetooth automatically disconnects the internal NTRIP client** to prevent data conflicts on the receiver's serial port.
> * To use the internal NTRIP client again, you must manually disable Bluetooth and save.

#### **A. Wi-Fi Configuration (for Internal NTRIP)**
Configures the ESP32's connection to an external Wi-Fi network (e.g., a phone hotspot or building router) to access internet-based corrections.
* **Enable Wi-Fi Client:** Check this box to allow the ESP32 to connect to the internet. If unchecked, the system remains in offline Access Point mode (AP).
* **Credentials:** Enter the SSID and Password of the target network.
* **Action:** Click **"Save and Reboot"**. 
    * *Effect:* The ESP32 will restart immediately to apply the new network settings. You will lose connection to the interface for about 10-15 seconds. You may need to reconnect your device to the `DUALY_CONFIG` network if the station connection fails.

#### **B. Bluetooth Relay (BLE)**
Configures the ESP32 to act as a wireless serial bridge (Service Name: `Dualy-GNSS`).
* **Technology:** This system uses **Bluetooth Low Energy (BLE)**, not Classic Bluetooth. 
    * **Do NOT pair** via your phone's main Bluetooth settings menu; it will fail or not find the device. 
    * **Connection:** Connect directly **inside** the target application.
* **Recommended Apps:**
    * **SW Maps:** Highly recommended for stability and NTRIP handling.
    * **nRF Connect:** Useful for initial debugging to verify the BLE signal is visible.
    * *Note:* While Lefebure NTRIP Client is supported, tests have shown **SW Maps** to be significantly more stable with this implementation.
* **Function:** * **Tx (NMEA):** Broadcasts position data to connected mobile apps 
    * **Rx (RTCM):** Receives corrections from a mobile phone and injects them into the receiver.
* **Action:** Click **"Apply Bluetooth Settings"**.
    * *Effect:* This applies the change instantly without a reboot. 
    * *Visual Feedback:* Check the Bluetooth icon in the top status bar. If it does not update immediately, **refresh the page** in your browser to verify the new state.

#### **C. Positioning Mode**
Defines the strict operating limits for the GNSS engine.
* **Mode Selection:** Select the allowed accuracy level (e.g., `StandAlone`, `RTK Fixed`, `DGNSS`). This is useful to prevent the receiver from outputting a position if the required precision (e.g., RTK) is not met.
* **Action:** Click **"Apply Position Mode"**.
    * *Effect:* Sends the `setPVTMode` command to the mosaic-g5. A confirmation message "Position mode command sent" will appear below the form.

#### **D. NTRIP Caster Configuration**
Sets up the internal client to fetch corrections over the internet (requires Wi-Fi).
* **Parameters:** Enter the Host (IP/URL), Port, Mountpoint, User, and Password provided by your correction service (e.g., CentipedeRTK, Rtk2Go, flepos.vlaanderen.be).
* **Fetch Button:** If you have entered a Host and Port, click **"Fetch"** to download the source table and select a Mountpoint from the dropdown list instead of typing it manually.
* **Action:** Click **"Save NTRIP Config"**.
    * *Effect:* Saves credentials to the ESP32's non-volatile memory. To start the actual data stream, go to the Dashboard or use the "Connect" button in the NTRIP status area.

---
## Trade-Off Analysis

This section outlines the rationale behind the architectural choices, the challenges encountered during development, and the roadmap for future improvements.

###  Language & Platform Choice
**Why C++ (Arduino/PlatformIO) over Python (MicroPython) or Rust?**
* **Performance:** The mosaic-g5 outputs data at high rates (up to 100Hz). Processing binary SBF streams byte-by-byte requires the raw speed of compiled C++. MicroPython's garbage collection would introduce unacceptable jitter/latency in the data stream.
* **Ecosystem:** The ESP32 Arduino core provides mature libraries for Wi-Fi, WebSockets, and BLE. While Rust is memory-safe, its ecosystem for ESP32 is still maturing and lacks the specific driver support needed for rapid internship development.
* **Direct Hardware Control:** C++ allows precise control over UART buffers and memory interrupts, crucial for parsing serial data without loss.

### Architecture: Why Dual-Core?
**The Challenge:**
Initially, the project ran on a single core. We observed that when the system was parsing a heavy SBF stream (high CPU load), the Web Interface would become unresponsive, or the Wi-Fi connection would drop due to "Watchdog Timer" timeouts.

**The Solution:**
We utilized the ESP32-S3's dual-core architecture to separate **Real-Time Processing** from **User Interface** tasks.
* **Core 0 (Task `Core0Loop`):** This core is dedicated to "Hard Real-Time" tasks. It continuously polls the UART buffer, runs the SBF state machine, and handles the TCP/NTRIP socket. It has higher priority to ensure no GNSS byte is ever dropped.
* **Core 1 (Main Loop):** This core handles "Soft Real-Time" tasks: serving the HTML page, managing WebSocket clients, and updating the BLE advertising.
* **Synchronization:** To prevent memory corruption when both cores try to access the WebSocket buffer simultaneously, a **Mutex (Semaphore)** (`webSocketMutex`) is used. This ensures thread safety.

### Frontend Strategy: SVG vs. PNG & SPA
**Vector Graphics (SVG):**
The Quality Indicators and Status Icons use inline SVG instead of PNG images.
* **Scalability:** SVG scales perfectly on any screen size (from smartphone to 4K monitor) without pixelation.
* **Dynamic Styling:** Because the SVG code is embedded in the HTML, we can manipulate it via CSS/JS. For example, the `fill` color of the status LEDs changes from Red to Green instantly by toggling a CSS class (`.active`). This is impossible with a static PNG.

**Single Page Application (SPA):**
The interface loads once. All updates happen via **WebSockets** (JSON payloads). This avoids refreshing the page, reducing the bandwidth load on the ESP32 and providing a fluid, "App-like" experience on mobile devices.

### Critical Logic: SBF Parsing State Machine
In `Septentrio_Arduino_driver.cpp`, we implemented a **Finite State Machine (FSM)** rather than simple buffer reading.
* **Why?** Serial data arrives asynchronously. We cannot guarantee that a full SBF block arrives in a single `read()` cycle.
* **Logic:** The FSM tracks the parsing stage (`WAITING_SYNC1` -> `WAITING_SYNC2` -> `READING_HEADER` -> `READING_PAYLOAD`).
* **Benefit:** This approach makes the parser robust against fragmented packets and data noise. If a checksum fails, the parser immediately resets to search for the next Sync bytes (`$@`), ensuring quick recovery.
### Future Improvements (Roadmap)
While functional, the code has room for optimization:

1.  **DMA (Direct Memory Access) for UART:** Currently, the CPU polls the serial port. Using DMA would allow the UART hardware to write directly to RAM, freeing up CPU cycles on Core 0.
2.  **HTTPS & WSS:** The current implementation uses unencrypted HTTP/WS. For a commercial deployment, adding TLS (SSL) encryption is mandatory to secure credentials.
3.  **Config Persistence Refactoring:** Currently, some settings are hardcoded or loosely stored. A dedicated `ConfigManager` class using the ESP32 `Preferences` API more extensively would make the system more robust against power losses.

---


## Technical Implementation

### 1. Software Foundation: The Eridano Legacy
The GNSS driver logic is an evolution of the **Eridano** library, originally developed internally at Septentrio. For the **SeptenUI** project, this library underwent significant refactoring to support hybrid operation (SBF + NMEA + NTRIP).

**Key Refactoring & Modifications:**
* **`Septentrio_Arduino_driver.cpp` / `.h`:**
    * **NTRIP Client:** Added a new `SEPTENTRIO_NTRIP` class capable of handling TCP streams, Base64 authentication, and periodic GGA transmission back to the caster.
    * **Hybrid Parsing:** Implemented a secondary NMEA state machine running in parallel with the SBF parser to handle mixed data streams without blocking.
    * **Performance:** Replaced dynamic CRC calculation with a pre-computed lookup table to reduce CPU cycles during high-frequency parsing.
* **`septentrio_structs.h`:**
    * **Buffer Expansion:** Increased `SBF_BUFFER_MAX_SIZE` to **512 bytes** to accommodate large complex blocks like `MeasEpoch` (Satellite Signal-to-Noise data) which were previously truncated.

### 2. Task Scheduling & Concurrency
The firmware maps specific functionalities to the ESP32-S3's hardware cores to guarantee signal integrity.

| Core | Task Name | Responsibilities |
| :--- | :--- | :--- |
| **Core 0** | `Core0Loop` | **Hard Real-Time:** <br>1. UART Interrupt handling (Reading byte-by-byte).<br>2. SBF & NMEA State Machine execution.<br>3. NTRIP TCP Stream management (Reading RTCM from Wi-Fi). |
| **Core 1** | `loop()` | **Soft Real-Time / UI:** <br>1. Hosting the AsyncWebServer (HTTP).<br>2. Broadcasting WebSocket JSON packets.<br>3. BLE Advertising and Characteristic updates. |

> **Concurrency Control:** A FreeRTOS Mutex (`webSocketMutex`) is implemented to prevent race conditions when the high-priority Core 0 tries to push data to the WebSocket buffer managed by Core 1.

### 3. SBF Decoder Logic
The driver processes the binary stream using a strictly defined sequence to extract navigation data.

**Parsed SBF Blocks:**
The system specifically filters and decodes the following Septentrio Binary Format blocks:
* **Positioning:** `PVTGeodetic` (ID 4007), `PosCovGeodetic` (ID 5906), `VelCovGeodetic` (ID 5908).
* **Attitude:** `AttEuler` (ID 5938), `AttCovEuler` (ID 5939).
* **Signal Analysis:** `QualityInd` (ID 4082), `RFStatus` (ID 4092), `SatVisibility` (ID 4012), `MeasEpoch` (ID 4027).
* **System:** `ReceiverStatus` (ID 4014).

**Parsing Pipeline:**
1.  **Sync:** Detect `$@`.
2.  **Header:** Read ID and Length.
3.  **Payload:** Buffer data.
4.  **Verification:** Compute CRC16; if valid, trigger specific callback functions (e.g., `parseAndBroadcastPVT`).

### 4. Data Protocols
**Internal Communication (ESP32 <-> Browser):**
* **Format:** JSON over WebSockets (Library: ArduinoJson v7).
* **Update Rate:** Up to 10Hz.
* **Payload Structure:**
    ```json
    {
      "type": "gga_update",
      "lat": 50.835,
      "lon": 4.351,
      "fix": 4,
      "sats_in_use": 12
    }
    ```

**External Communication (ESP32 <-> Mobile Apps):**
* **Protocol:** Bluetooth Low Energy (BLE).
* **Service:** Nordic UART Service (NUS) - UUID `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`.
* **Characteristics:**
    * `TX`: Sends NMEA sentences (ASCII) to the phone.
    * `RX`: Receives raw RTCM binary data from the phone.

---

##  Troubleshooting

### A. Common Issues
| Issue | Potential Cause | Solution |
| :--- | :--- | :--- |
| **"WS Not Connected"** | Browser cannot reach ESP32. | Check Wi-Fi connection. Ensure you are on the same network. |
| **No GNSS Data** | Wiring issue or baud rate mismatch. | Verify RX/TX swap (GPIO 21/12). Baud rate must be 115200. |
| **NTRIP not connecting** | Wrong credentials or no Internet. | Check "Config" tab details. Verify ESP32 has Internet access. |
| **Interface Empty** | SPIFFS failure. | Re-upload the Filesystem Image. |

### B. The "SUF Mode" Loop (Hardware Warning)
**Symptom:** After uploading new code to the ESP32, the mosaic-g5 stops outputting data and enters "Software Upgrade Fallback" (SUF) mode for 200 seconds.
**Cause:** In the current wiring configuration, the ESP32's `RESET` pin is tied to the mosaic-g5's `nRST/UPGRADE` pin. Flashing the ESP32 toggles this pin, inadvertently triggering the receiver's upgrade mode.
**Solutions:**
1.  **Soft Fix:** Open the **Expert Console** in SeptenUI, type the command `reset`, and press Enter. This forces the receiver to reboot into normal mode immediately. Then, refresh the webpage.
2.  **Hard Fix:** Physically disconnect the wire between the ESP32 Reset pin and the mosaic-g5 Upgrade pin if remote flashing is not required.

### C. Advanced Debugging
The firmware contains extensive serial logging for debugging purposes. 
* **Tool:** Use the **Serial Monitor** in Arduino IDE or VS Code (Baud Rate: 115200).
* **What to look for:** The logs provide real-time feedback on Wi-Fi connection attempts, raw NTRIP byte counts, BLE client subscriptions, and SBF parsing errors.
---


## Reference Documentation

This project was built based on the extensive documentation provided by Septentrio. For deeper technical explanations regarding SBF block structures, specific command definitions (SDS), or GNSS theory, please refer to the official guides:

1.  **mosaic-g5 Reference Guide:** Essential for understanding the low-level binary SBF blocks (e.g., `PVTGeodetic`, `QualityInd`) and the command line interface (CLI) syntax used in the Expert Console.
2.  **RxControl Manual:** The logic for parsing signals, visualising skyplots, and managing connection streams in **SeptenUI** was heavily inspired by the workflows standard in Septentrio's RxControl software.
