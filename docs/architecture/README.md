# Architecture

<div align="center">

<img src="../images/webui-logo.png" alt="WebUI Logo" width="220">

### Firmware organization, runtime model and data flow

[← Documentation Hub](../README.md) · [Hardware & Board Profiles](../hardware/README.md) · [Connectivity](../connectivity/README.md)

</div>

---

## Overview

WebUI is structured as a modular embedded application running on an **ESP32-S3** and interfacing with a **Septentrio mosaic** GNSS receiver.

The architecture is designed around four main goals:

- keep the browser interface available as early as possible after boot
- keep GNSS processing independent from WebUI rendering
- isolate connectivity services such as Wi-Fi, NTRIP and BLE
- separate reusable application logic from board-specific hardware

The ESP32 therefore acts as both:

1. a **GNSS data processor and control bridge**
2. an **embedded web server and connectivity gateway**

The browser never communicates directly with the mosaic receiver.  
All receiver data and commands pass through the ESP32 application.

---

## System Architecture

<div align="center">

<img src="../images/system-architecture.png"
     alt="WebUI System Architecture"
     width="1000">

</div>

At a high level:

```text
GNSS signals
     ↓
Septentrio mosaic
     ↓
SBF / NMEA over UART
     ↓
ESP32-S3
     ├── GNSS parsing
     ├── Receiver control
     ├── Wi-Fi AP / STA
     ├── NTRIP client
     ├── BLE
     ├── HTTP server
     ├── WebSocket server
     └── SPIFFS-hosted frontend
     ↓
Browser WebUI
```

The ESP32 also supports the NTRIP correction path:

```text
mosaic → GGA → ESP32 → NTRIP caster
                         ↓
mosaic ← RTCM ← ESP32 ← corrections
```

For the full connectivity behaviour, see the [Connectivity Guide](../connectivity/README.md).

---

## Architectural Principles

### 1. WebUI-first startup

The browser interface should become reachable without waiting for every hardware or network subsystem to complete initialization.

The startup strategy therefore prioritizes:

```text
ESP32 boot
   ↓
SPIFFS
   ↓
Wi-Fi Access Point
   ↓
HTTP + WebSocket services
   ↓
WebUI available
   ↓
Background / parallel subsystem initialization
```

GNSS, Wi-Fi Station mode, BLE and other services can continue initializing after the local WebUI is already reachable.

This reduces perceived startup time and makes debugging easier because the user can access the interface even if an external network or receiver is not immediately ready.

---

### 2. Modular responsibilities

Each subsystem owns a defined responsibility.

The application should avoid putting GNSS parsing, network logic, frontend communication and board-specific hardware handling into one monolithic source file.

Instead, the firmware is organized into focused modules.

---

### 3. Explicit shared state

Runtime state shared across modules is centralized rather than recreated independently.

Examples include:

- GNSS driver instances
- NTRIP driver instance
- Wi-Fi configuration
- NTRIP configuration
- operating mode
- GNSS serial readiness
- receiver stream readiness
- RTCM activity state

This shared state is exposed through the application-level globals and configuration headers.

---

### 4. Board-specific hardware stays optional

Hardware features that do not exist on every target should not block or destabilize the generic application.

Optional peripherals are therefore enabled through board capabilities.

For example:

```cpp
#if WEBUI_HAS_KTD2026
initLedDrivers();
#endif
```

See the [Hardware & Board Profiles Guide](../hardware/README.md).

---

## Source Tree

The firmware is organized by subsystem:

```text
src/
├── ble/
├── config/
├── gnss/
├── led/
├── logging/
├── ntrip/
├── utils/
├── web/
└── websocket/
```

The shared headers are stored under:

```text
include/
```

A simplified responsibility map is:

| Module | Responsibility |
|---|---|
| `gnss/` | UART communication, SBF/NMEA parsing, receiver stream handling |
| `ntrip/` | Caster connection, sourcetable retrieval, RTCM handling |
| `web/` | HTTP server and SPIFFS-hosted frontend |
| `websocket/` | Browser ↔ ESP32 real-time communication |
| `ble/` | Bluetooth Low Energy services and data relay |
| `config/` | Persistent application configuration |
| `logging/` | Receiver logging control |
| `led/` | Optional board LED hardware |
| `utils/` | Shared helpers and diagnostics |

The project root also contains:

```text
data_src/   Editable frontend sources
data/       Compressed SPIFFS-ready assets
test/       Development and validation code
```

---

## Main Application Entry Point

The firmware starts in:

[`src/main.cpp`](../../src/main.cpp)

Its responsibility is orchestration rather than implementing every subsystem directly.

Conceptually, startup follows:

```text
setup()
  │
  ├── initialize serial diagnostics
  ├── initialize synchronization objects
  ├── mount SPIFFS
  ├── load persistent configuration
  ├── initialize Wi-Fi Access Point
  ├── initialize mDNS
  ├── start HTTP server
  ├── start WebSocket server
  ├── start delayed Wi-Fi STA task
  ├── start GNSS receiver task
  ├── initialize optional board hardware
  └── start BLE initialization
```

The Arduino `loop()` then remains lightweight and services runtime activities such as:

- WebSocket processing
- periodic Wi-Fi status
- application heartbeat
- optional LED state updates

Longer or independent operations are kept outside the main loop where possible.

---

## Execution Model

WebUI uses the ESP32-S3's FreeRTOS environment to separate time-sensitive GNSS processing from browser and connectivity responsibilities.

A simplified model is:

```text
              ESP32-S3
                  │
        ┌─────────┴─────────┐
        │                   │
        ▼                   ▼
 Application / Web      GNSS Receiver Task
      services
        │                   │
        │                   ├── UART receive
        │                   ├── SBF parsing
        │                   ├── NMEA parsing
        │                   ├── receiver console
        │                   ├── NTRIP handling
        │                   └── receiver state updates
        │
        ├── WebSocket
        ├── HTTP
        ├── Wi-Fi
        ├── BLE startup
        └── UI state publication
```

The exact core affinity and task configuration should be checked in the current firmware source before changing scheduling behaviour.

> [!IMPORTANT]
> The architectural objective is more important than a specific core number: GNSS serial processing must not be starved by browser traffic or slower network operations.

---

## GNSS Receiver Task

The GNSS task performs the receiver-facing work.

Its initialization sequence is approximately:

```text
GNSS task starts
      ↓
Initialize UART
      ↓
Attach Septentrio driver
      ↓
Attach NTRIP driver to receiver stream
      ↓
Wait for receiver readiness
      ↓
Configure required receiver streams
      ↓
Continuous processing loop
```

During normal operation, the task repeatedly handles:

- UART bytes
- SBF synchronization and parsing
- NMEA parsing
- receiver console traffic
- BLE GNSS relay where enabled
- NTRIP data flow where enabled

This design keeps receiver traffic flowing even when the browser is disconnected.

---

## UART Layer

The current receiver path uses an ESP32 hardware serial interface.

Conceptually:

```cpp
Serial2.begin(
    115200,
    SERIAL_8N1,
    MOSAIC_RX_PIN,
    MOSAIC_TX_PIN
);
```

The serial object is then provided to the Septentrio GNSS and NTRIP driver instances.

Important distinction:

```text
ESP32 Serial2  ≠  mosaic COM2
```

`Serial2` identifies the ESP32 UART peripheral.

`COM1` or `COM2` identifies the physical communication port on the Septentrio receiver.

The receiver stream configuration must match the mosaic port actually wired to the ESP32.

For wiring details, see [Hardware & Board Profiles](../hardware/README.md).

---

## GNSS Data Pipeline

Each incoming UART byte is processed by the GNSS parser.

Conceptually:

```text
UART byte
   │
   ├──→ SBF parser
   │       ↓
   │   validated SBF block
   │       ↓
   │   application state
   │
   └──→ NMEA parser
           ↓
       complete sentence
           ↓
       application / NTRIP / BLE
```

The application does not wait for one protocol before checking the other.

This allows SBF and NMEA streams to coexist on the same receiver UART.

---

## SBF Processing

SBF provides the richer receiver information used throughout WebUI.

Relevant receiver information includes blocks such as:

```text
PVTGeodetic
QualityInd
RFStatus
ReceiverStatus
DOP
ReceiverTime
SatVisibility
ChannelStatus
MeasEpoch
EndOfPVT
PosCovGeodetic
VelCovGeodetic
```

Parsed data is converted into application state that can then be serialized for the WebUI.

The browser does not parse raw SBF itself.

This keeps:

- binary parsing on the embedded side
- frontend logic simpler
- receiver-specific protocol details out of JavaScript

For detailed GNSS behaviour, see the [GNSS Integration Guide](../gnss/README.md).

---

## NMEA Processing

NMEA is handled alongside SBF.

The most important sentence for the correction workflow is:

```text
GGA
```

A valid GGA can be forwarded to the NTRIP client when the selected correction service requires rover position.

Conceptually:

```text
mosaic
  ↓
GGA sentence
  ↓
ESP32 parser
  ↓
NTRIP client
  ↓
caster
```

NMEA can also be relayed over BLE where that feature is enabled.

---

## Receiver Stream Configuration

WebUI actively requests receiver output required by the interface.

This avoids depending entirely on whatever receiver configuration happened to exist before boot.

The stream configuration includes:

- SBF blocks for PVT and receiver health
- satellite and channel information
- covariance / velocity information
- NMEA GGA

A representative conceptual configuration is:

```text
Stream 1 → PVT / quality / RF / receiver status
Stream 2 → satellite / channel / measurement information
Stream 3 → position and velocity covariance
Stream 4 → NMEA GGA
```

The physical target must be the mosaic COM port connected to the ESP32.

---

## Application State

The parser does not send every raw byte directly to the browser.

Instead, WebUI maintains structured state representing the most recent receiver information.

Examples:

```text
Position
Velocity
Fix / PVT mode
Satellite count
Quality indicators
RF status
Receiver status
DOP
Time
Jamming state
Connectivity state
RTCM activity
```

This gives the browser a stable interface even though the underlying receiver protocols are binary and asynchronous.

---

## WebSocket Architecture

WebSocket is the primary real-time communication path between the ESP32 and the browser.

Conceptually:

```text
          Browser
             │
     JSON over WebSocket
             │
             ▼
      WebSocket Manager
         │          │
         │          └── user commands
         │
         └── telemetry / status
             │
             ▼
      Application modules
```

The browser can request actions such as:

- configuration changes
- Wi-Fi operations
- NTRIP operations
- receiver commands
- logging control
- full status refresh

The ESP32 sends:

- GNSS state
- connectivity state
- system status
- mountpoint lists
- receiver feedback
- correction activity

---

## WebSocket Synchronization

Because WebSocket communication can originate from multiple runtime contexts, synchronization is required.

A mutex is used to protect access to shared WebSocket operations.

One important architectural rule is:

> A callback that already executes inside a locked WebSocket context must not attempt to reacquire the same non-recursive mutex.

This matters for operations that perform work and then immediately return data to the requesting client.

For example, mountpoint fetching is better structured as:

```text
Browser request
      ↓
WebSocket callback
      ↓
Fetch NTRIP sourcetable
      ↓
Build response
      ↓
Send directly to requesting client
```

rather than nesting a broadcast path that tries to lock the same WebSocket mutex again.

This avoids self-deadlock / dropped-response behaviour.

---

## HTTP and SPIFFS

The frontend is hosted directly on the ESP32.

Source assets live in:

```text
data_src/
```

The build helper:

[`compress_webui.py`](../../compress_webui.py)

generates compressed assets in:

```text
data/
```

Those assets are uploaded to SPIFFS separately from the firmware.

Runtime path:

```text
Browser HTTP request
      ↓
ESPAsyncWebServer
      ↓
SPIFFS
      ↓
index.html.gz / CSS / JS / assets
      ↓
Browser
```

Once loaded, the browser uses WebSocket for live telemetry and commands.

---

## Frontend / Firmware Separation

WebUI deliberately separates browser assets from embedded C++ code.

```text
Firmware
   │
   ├── receiver logic
   ├── connectivity
   ├── parsing
   ├── API / WebSocket
   └── hardware control

Frontend
   │
   ├── HTML
   ├── CSS
   ├── JavaScript
   ├── charts
   └── interface assets
```

This means a frontend-only change normally requires:

```text
Edit data_src/
      ↓
Run compress_webui.py
      ↓
Upload SPIFFS filesystem
```

without recompiling the C++ firmware.

See [Getting Started](../getting-started/README.md).

---

## Wi-Fi Architecture

WebUI uses two complementary Wi-Fi roles.

### Access Point

The ESP32 provides its own local network:

```text
WEBUI_CONFIG
```

with the default local address:

```text
192.168.3.1
```

This provides direct local access to the browser interface.

### Station

The ESP32 can also connect to an external 2.4 GHz Wi-Fi network.

This provides Internet access for services such as NTRIP.

Together:

```text
Laptop / phone
      │
      │ AP
      ▼
   ESP32
      │
      │ STA
      ▼
External Wi-Fi
      │
      ▼
   Internet
```

The AP can remain available while Station mode is active.

---

## Delayed Wi-Fi Station Initialization

Station connection is intentionally separated from initial WebUI availability.

Conceptually:

```text
Boot
 ↓
Start AP
 ↓
Start WebUI
 ↓
User can connect locally
 ↓
Start / request STA connection
 ↓
Internet services become available
```

This prevents a slow or unavailable external Wi-Fi network from blocking the local interface during boot.

---

## NTRIP Architecture

The ESP32 contains the NTRIP client.

The full correction path is:

```text
mosaic receiver
      │
      │ GGA
      ▼
    ESP32
      │
      │ Internet / NTRIP
      ▼
 NTRIP caster
      │
      │ RTCM
      ▼
    ESP32
      │
      │ UART
      ▼
mosaic receiver
```

The NTRIP module is responsible for:

- caster connection
- sourcetable retrieval
- mountpoint selection support
- GGA forwarding
- RTCM reception
- connection monitoring
- reconnect behaviour

See the [Connectivity Guide](../connectivity/README.md).

---

## Correction-State Semantics

A network socket being open does not necessarily prove that GNSS corrections are valid.

The architecture therefore distinguishes between several levels of success:

```text
Wi-Fi connected
      ↓
Caster reachable
      ↓
NTRIP session accepted
      ↓
RTCM bytes received
      ↓
RTCM forwarded
      ↓
Receiver correction state changes
```

For end-to-end validation, see [Testing & Validation](../testing/README.md).

---

## BLE Architecture

BLE is an optional parallel output / connectivity path.

The BLE service can operate without changing the primary browser data path.

Conceptually:

```text
GNSS data
   │
   ├──→ WebUI / WebSocket
   │
   └──→ BLE relay
```

This keeps BLE additive rather than making the WebUI dependent on Bluetooth.

---

## Persistent Configuration

Runtime configuration is stored using the ESP32 persistent storage facilities.

Typical persistent values include:

```text
Wi-Fi SSID
Wi-Fi password
Wi-Fi STA enabled state
NTRIP host
NTRIP port
NTRIP mountpoint
NTRIP username
NTRIP password
NTRIP auto-connect
Bluetooth enabled state
```

At startup:

```text
Boot
 ↓
Load stored configuration
 ↓
Initialize local services
 ↓
Apply enabled runtime behaviour
```

Sensitive configuration should never be printed in plaintext diagnostics.

---

## Operating Mode

The application tracks its operating state explicitly.

A representative operating-mode model includes:

```cpp
enum OperatingMode {
    MODE_IDLE,
    MODE_INTERNAL_NTRIP
};
```

The operating mode determines which higher-level services are active.

This is preferable to scattering unrelated Boolean conditions throughout the application because it makes future operating modes easier to add and reason about.

---

## Runtime Readiness Flags

Some subsystems should not start until their dependencies are ready.

The application therefore tracks readiness explicitly, for example:

```text
gnssSerialReady
receiverStreamsReady
```

This supports sequencing such as:

```text
UART ready?
   │
   └── No → wait
   │
   └── Yes
          ↓
Receiver streams configured?
   │
   └── No → configure
   │
   └── Yes
          ↓
Normal GNSS processing
```

Explicit readiness states make startup behaviour easier to diagnose than relying only on elapsed delays.

---

## Data Ownership

A useful rule for extending WebUI is:

```text
Raw protocol data
      ↓
Owned by GNSS / connectivity module
      ↓
Converted into application state
      ↓
Serialized by WebSocket layer
      ↓
Rendered by browser
```

Avoid letting the frontend become responsible for interpreting low-level receiver protocols.

Likewise, avoid making the GNSS parser responsible for browser-specific formatting.

---

## Error Isolation

The architecture is designed so that one unavailable subsystem does not necessarily make the whole product unusable.

Examples:

| Failure | Expected architectural behaviour |
|---|---|
| No Internet | Local WebUI should remain available |
| NTRIP caster unavailable | GNSS monitoring should continue |
| BLE unavailable | Browser interface should continue |
| Optional LED driver absent | Generic firmware should continue |
| Browser disconnected | GNSS parsing should continue |
| mosaic not connected | WebUI should still expose system/connectivity diagnostics |
| External Wi-Fi unavailable | AP should remain usable |

This separation is important for field debugging.

---

## Startup Sequence

A simplified end-to-end boot sequence is:

```text
Power-on / Reset
      ↓
Serial diagnostics
      ↓
Synchronization objects
      ↓
SPIFFS mount
      ↓
Load configuration
      ↓
Wi-Fi AP
      ↓
mDNS
      ↓
HTTP server
      ↓
WebSocket server
      ↓
──────── WebUI reachable ────────
      ↓
Start GNSS task
      ↓
Initialize receiver UART
      ↓
Configure receiver streams
      ↓
Start optional BLE
      ↓
Start / request Wi-Fi STA
      ↓
Enable NTRIP when requested
      ↓
Normal runtime
```

The key boundary is:

```text
WebUI reachable
```

The architecture attempts to reach that state early.

---

## Runtime Data Flow

### GNSS → Browser

```text
mosaic
  ↓ UART
ESP32 GNSS parser
  ↓
Application state
  ↓
JSON serialization
  ↓
WebSocket
  ↓
Browser
```

### Browser → Receiver

```text
Browser
  ↓ WebSocket
ESP32 command handler
  ↓
Receiver command
  ↓ UART
mosaic
```

### NTRIP → Receiver

```text
NTRIP caster
  ↓ RTCM
ESP32 NTRIP client
  ↓
Receiver UART
  ↓
mosaic
```

### Receiver → NTRIP

```text
mosaic
  ↓ GGA
ESP32 parser
  ↓
NTRIP client
  ↓
Caster
```

---

## Architecture Boundaries

The following separations should be preserved when extending the project.

### GNSS vs. WebUI

Good:

```text
GNSS parser → structured state → WebSocket → browser
```

Avoid:

```text
GNSS parser → browser-specific HTML / DOM assumptions
```

### Board hardware vs. generic logic

Good:

```text
Capability flag → optional board module
```

Avoid:

```text
Dualy-specific peripheral required by generic startup
```

### Connectivity vs. receiver parser

Good:

```text
NTRIP module receives RTCM → forwards bytes to receiver stream
```

Avoid:

```text
NTRIP implementation mixed into SBF parsing code
```

### Frontend vs. firmware

Good:

```text
JSON contract between ESP32 and browser
```

Avoid:

```text
frontend depending on internal C++ implementation details
```

---

## Extending the Architecture

When adding a feature, first identify which layer owns it.

| Feature type | Preferred location |
|---|---|
| New SBF block | `gnss/` |
| New receiver command | GNSS / command handling |
| New Wi-Fi behaviour | connectivity / Wi-Fi module |
| New NTRIP behaviour | `ntrip/` |
| New browser telemetry field | application state + WebSocket + frontend |
| New board peripheral | board capability + dedicated hardware module |
| New persistent setting | `config/` |
| New BLE service | `ble/` |
| New WebUI page/widget | `data_src/` |

A feature may cross several layers, but each layer should still own only its part of the implementation.

---

## Architecture Checklist for New Features

Before merging a new feature, verify:

- Does it belong to the correct module?
- Does it block startup unnecessarily?
- Does it depend on hardware that may not exist on every board?
- Does it require a capability flag?
- Does it introduce shared state?
- Is shared state synchronized where necessary?
- Does it expose a stable WebSocket/JSON contract?
- Does it continue working when the browser disconnects?
- Does it continue working when Internet access disappears?
- Does it log enough information to diagnose failures?
- Does it avoid logging credentials or other secrets?
- Can it be validated independently?

---

## Related Documentation

| Topic | Guide |
|---|---|
| Build and flash | [Getting Started](../getting-started/README.md) |
| Board profiles and UART wiring | [Hardware & Board Profiles](../hardware/README.md) |
| SBF, NMEA and receiver streams | [GNSS Integration](../gnss/README.md) |
| Wi-Fi, NTRIP, RTCM and BLE | [Connectivity](../connectivity/README.md) |
| End-to-end validation | [Testing & Validation](../testing/README.md) |
| Runtime diagnosis | [Troubleshooting](../troubleshooting/README.md) |

---

<div align="center">

### WebUI Architecture

**Receiver data stays real-time. Connectivity stays modular. Hardware stays explicit.**

[← Hardware & Board Profiles](../hardware/README.md) · [Documentation Hub](../README.md) · [Next: Connectivity →](../connectivity/README.md)

</div>
