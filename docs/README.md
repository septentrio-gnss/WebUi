# WebUI Documentation

<div align="center">

<img src="images/webui-logo.png" alt="WebUI Logo" width="220">

### Documentation Hub

Technical documentation for the **WebUI embedded GNSS monitoring and control platform**.

[← Back to Main README](../README.md)

</div>

---

## Overview

This directory contains the technical and user documentation for **WebUI**, an embedded browser-based interface designed for integrations combining:

- an **ESP32-S3**
- a **Septentrio mosaic GNSS receiver**
- a browser-based monitoring and control interface
- optional **Wi-Fi, NTRIP, RTCM and Bluetooth** connectivity

WebUI was initially developed and validated using **Dualy** as the reference implementation, while the software architecture is designed to remain reusable across compatible ESP32 + Septentrio mosaic platforms.

The documentation is intentionally separated by topic so that users can quickly find the information relevant to their task without navigating through a single large README.

---

## Documentation Map

| Guide | Purpose |
|---|---|
| **[Getting Started](getting-started/README.md)** | Install the development environment, select a board profile, build the project, upload the firmware and launch WebUI for the first time |
| **[User Guide](user-guide/README.md)** | Use the dashboard, GNSS monitoring tools, Expert Console, logging controls and runtime configuration |
| **[Hardware & Board Profiles](hardware/README.md)** | Understand supported ESP32 hardware, Septentrio mosaic connections, UART wiring, optional peripherals and board-specific configuration |
| **[Architecture](architecture/README.md)** | Understand the firmware organization, FreeRTOS execution model, startup sequence, modules and internal data flow |
| **[Connectivity](connectivity/README.md)** | Configure Wi-Fi AP/STA, NTRIP, mountpoints, RTCM correction transport and Bluetooth |
| **[GNSS Integration](gnss/README.md)** | Understand Septentrio mosaic communication, SBF/NMEA parsing, receiver streams and GNSS information exposed by WebUI |
| **[Testing & Validation](testing/README.md)** | Validate WebUI functionality, receiver communication, RxControl consistency, connectivity and the correction chain |
| **[Troubleshooting](troubleshooting/README.md)** | Diagnose common firmware, SPIFFS, UART, Wi-Fi, GNSS, NTRIP and BLE issues |

---

## Where Should I Start?

<table>
<tr>
<td width="33%" valign="top">

### 🚀 First-time setup

Start here if you have just cloned the repository and want to run WebUI on hardware.

**Read:**  
[Getting Started →](getting-started/README.md)

Then continue with:

[Hardware & Board Profiles →](hardware/README.md)

</td>

<td width="33%" valign="top">

### 🧭 Using WebUI

Start here if the firmware is already running and you want to understand the interface.

**Read:**  
[User Guide →](user-guide/README.md)

For GNSS-specific information:

[GNSS Integration →](gnss/README.md)

</td>

<td width="33%" valign="top">

### 🛠 Development & integration

Start here if you want to modify, extend or port WebUI.

**Read:**  
[Architecture →](architecture/README.md)

Then continue with:

[Connectivity →](connectivity/README.md)

</td>
</tr>
</table>

---

## Typical Workflows

### Deploy WebUI on a supported board

```text
Clone repository
      ↓
Select PlatformIO environment
      ↓
Build firmware
      ↓
Prepare WebUI assets
      ↓
Upload firmware
      ↓
Upload SPIFFS filesystem
      ↓
Connect to WEBUI_CONFIG
      ↓
Open WebUI in browser
```

See the [Getting Started Guide](getting-started/README.md).

---

### Integrate a new ESP32-based platform

```text
Check hardware compatibility
      ↓
Define board-specific parameters
      ↓
Configure UART pins
      ↓
Configure flash / PSRAM
      ↓
Enable optional board capabilities
      ↓
Build and flash
      ↓
Validate GNSS communication
```

See the [Hardware & Board Profiles Guide](hardware/README.md).

---

### Validate GNSS corrections

```text
GNSS receiver operational
      ↓
Valid GGA available
      ↓
ESP32 connected to Internet Wi-Fi
      ↓
NTRIP caster configured
      ↓
Mountpoint selected
      ↓
NTRIP connection established
      ↓
RTCM data received
      ↓
Corrections forwarded to receiver
      ↓
DGNSS / RTK behaviour verified
```

See the [Connectivity Guide](connectivity/README.md) and [Testing & Validation Guide](testing/README.md).

---

## Documentation Scope

The documentation separates four different levels of the project:

### Application

How the end user interacts with WebUI:

- GNSS monitoring
- receiver status
- satellite information
- configuration
- logging
- Expert Console

→ [User Guide](user-guide/README.md)

### GNSS Integration

How WebUI communicates with the Septentrio receiver:

- UART communication
- SBF blocks
- NMEA messages
- receiver stream configuration
- PVT information
- quality indicators

→ [GNSS Integration](gnss/README.md)

### Embedded Platform

How the application runs on the ESP32:

- board profiles
- hardware capabilities
- FreeRTOS tasks
- HTTP server
- WebSocket communication
- SPIFFS
- persistent configuration

→ [Hardware](hardware/README.md)  
→ [Architecture](architecture/README.md)

### External Connectivity

How the ESP32 communicates beyond the local receiver:

- Wi-Fi Access Point
- Wi-Fi Station mode
- AP + STA operation
- NTRIP
- RTCM
- Bluetooth Low Energy

→ [Connectivity](connectivity/README.md)

---

## Reference Implementation

**Dualy** is used as the primary reference implementation for WebUI development and validation.

It provides a complete integration of an ESP32-S3 with a Septentrio mosaic receiver and additional board-specific hardware.

However, Dualy-specific behaviour should remain isolated from the reusable WebUI core whenever possible.

The intended software model is:

```text
                    WebUI Core
                        │
          ┌─────────────┴─────────────┐
          │                           │
  Generic functionality       Board-specific layer
          │                           │
   GNSS / WebSocket /          Pins / peripherals /
   Wi-Fi / NTRIP / BLE         hardware capabilities
          │                           │
          └─────────────┬─────────────┘
                        │
                 Target Hardware
```

This separation allows the same WebUI architecture to be adapted to different compatible ESP32-S3 implementations.

---

## Validation Philosophy

A feature should not be considered fully validated only because it appears correctly in the browser.

Where applicable, validation should cover the complete data path:

```text
Physical receiver
      ↓
UART communication
      ↓
SBF / NMEA parsing
      ↓
ESP32 application state
      ↓
WebSocket transport
      ↓
Browser representation
```

For correction services, validation extends further:

```text
Receiver GGA
      ↓
ESP32
      ↓
NTRIP caster
      ↓
RTCM corrections
      ↓
ESP32
      ↓
Receiver
      ↓
Positioning solution
```

WebUI GNSS information should also be cross-checked against **Septentrio RxControl** where appropriate.

See the [Testing & Validation Guide](testing/README.md).

---

## Repository Documentation

Additional project-level documents are located at the repository root:

| File | Purpose |
|---|---|
| [`README.md`](../README.md) | Project overview and entry point |
| [`CONTRIBUTING.md`](../CONTRIBUTING.md) | Contribution guidelines |
| [`SECURITY.md`](../SECURITY.md) | Security reporting and security-related considerations |
| [`KNOWN_ISSUES.md`](../KNOWN_ISSUES.md) | Known limitations and unresolved issues |
| [`CHANGELOG.md`](../CHANGELOG.md) | Project changes and release history |
| [`platformio.ini`](../platformio.ini) | PlatformIO environments and board build configuration |

> The links above assume these repository-level files are present in the final public structure.

---

## Documentation Conventions

Throughout the documentation:

- `WebUI` refers to the complete embedded application and browser interface.
- `ESP32` refers to the embedded controller running WebUI.
- `mosaic` refers to a compatible Septentrio mosaic GNSS receiver.
- `SBF` refers to Septentrio Binary Format.
- `NMEA` refers to standard ASCII GNSS messages.
- `NTRIP` refers to Networked Transport of RTCM via Internet Protocol.
- `RTCM` refers to GNSS correction data transported to the receiver.
- `RxControl` refers to Septentrio's receiver control and monitoring software.
- `Dualy` refers to the primary reference hardware implementation.

Board-specific values such as UART pins, flash configuration or optional peripherals should always be checked against the selected PlatformIO environment and hardware profile rather than assumed globally.

---

## Need Help?

For installation or first-boot problems:

**[Getting Started →](getting-started/README.md)**

For runtime or hardware problems:

**[Troubleshooting →](troubleshooting/README.md)**

For questions about receiver data:

**[GNSS Integration →](gnss/README.md)**

For NTRIP, Wi-Fi or Bluetooth:

**[Connectivity →](connectivity/README.md)**

---

<div align="center">

### WebUI Documentation

**From hardware integration to GNSS data in the browser.**

[← Main README](../README.md) · [Getting Started](getting-started/README.md) · [Troubleshooting](troubleshooting/README.md)

</div>

