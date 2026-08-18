<div align="center">

<img src="docs/images/webui-logo.png" alt="WebUI Logo" width="330">

# WebUI

### Embedded GNSS Monitoring & Control for ESP32 + Septentrio mosaic

A lightweight, browser-based interface for monitoring, configuring and controlling Septentrio mosaic GNSS receivers directly from an ESP32.

**Professional GNSS visibility — directly from your browser.**

[Getting Started](docs/getting-started/README.md) • [User Guide](docs/user-guide/README.md) • [Architecture](docs/architecture/README.md) • [NTRIP & BLE](docs/connectivity/README.md) • [Testing](docs/testing/README.md)

</div>

---

## What is WebUI?

**WebUI** turns an ESP32-S3 into an embedded interface for a **Septentrio mosaic GNSS receiver**. The ESP32 receives SBF/NMEA data from the receiver and exposes the relevant information through a responsive browser interface.

> Septentrio RxControl remains the reference application for complete receiver configuration, firmware management and advanced diagnostics. WebUI focuses on embedded monitoring, field operation and integration.

---

## Highlights

- Real-time position, velocity and PVT mode
- Dynamic satellite skyplot
- RF, signal and receiver quality indicators
- Wi-Fi Access Point + Station mode
- WebSocket live telemetry
- Embedded NTRIP client with dynamic mountpoint retrieval
- BLE NMEA / RTCM relay
- Expert Console and receiver logging control
- RxControl cross-validation

---

## System Overview

```text
Septentrio mosaic
       │ SBF / NMEA / UART
       ▼
    ESP32-S3
       │ HTTP + WebSocket / Wi-Fi
       ▼
     Browser
       │
      WebUI
```

WebUI was originally developed and validated using **Dualy** as its reference embedded platform, while the architecture is designed to remain reusable across compatible ESP32 + mosaic integrations.

---

## Quick Start

```bash
git clone <repository-url>
cd WebUI
pio run
python compress_webui.py
pio run -t upload
pio run -t uploadfs
```

After boot:

```text
SSID: WEBUI_CONFIG
IP:   192.168.3.1
mDNS: http://webui.local
```

Open `http://192.168.3.1`.

[Full installation instructions →](docs/getting-started/README.md)

---

## Documentation

| Guide | Purpose |
|---|---|
| [Getting Started](docs/getting-started/README.md) | Installation, PlatformIO, flashing and first boot |
| [User Guide](docs/user-guide/README.md) | Dashboard, console, logging and configuration |
| [Architecture](docs/architecture/README.md) | Firmware structure, tasks and data flow |
| [Connectivity](docs/connectivity/README.md) | Wi-Fi, NTRIP, RTCM and BLE |
| [GNSS Integration](docs/gnss/README.md) | SBF/NMEA, parsed blocks and mosaic communication |
| [Testing](docs/testing/README.md) | RxControl comparison and validation |
| [Troubleshooting](docs/troubleshooting/README.md) | Common issues and recovery |

### [Open the complete Documentation Hub →](docs/README.md)

---

## Repository Overview

```text
WebUI/
├── src/
├── include/
├── data_src/
├── data/
├── test/
├── docs/
├── compress_webui.py
├── platformio.ini
└── README.md
```

---

## Project Status

Validated core capabilities include ESP32-hosted WebUI, Wi-Fi AP + STA, WebSocket telemetry, mosaic UART communication, SBF/NMEA parsing, positioning display, quality indicators, NTRIP configuration and mountpoint retrieval, BLE integration, logging controls, Expert Console and RxControl comparison.
