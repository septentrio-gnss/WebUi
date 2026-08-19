# User Guide

<div align="center">

<img src="../images/webui-logo.png" alt="WebUI Logo" width="220">

### Using the WebUI dashboard, controls and runtime configuration

[← Documentation Hub](../README.md) · [Getting Started](../getting-started/README.md) · [Troubleshooting](../troubleshooting/README.md)

</div>

---

## Overview

This guide explains how to use WebUI once the firmware is running.

It focuses on the browser interface only.  
For installation, wiring or protocol details, use the dedicated technical guides.

Open WebUI at:

```text
http://192.168.3.1
```

or, where mDNS is available:

```text
http://webui.local
```

---

## Dashboard

The dashboard provides the main live view of the receiver.

Typical information includes:

- position
- velocity
- PVT solution mode
- satellite visibility
- skyplot
- quality indicators
- RF / receiver status
- connectivity state
- logging state
- correction activity

<div align="center">

<img src="../images/dashboard-live.png"
     alt="WebUI Dashboard"
     width="950">

</div>

Values update through WebSocket without reloading the page.

---

## Connection Status

Before interpreting GNSS data, verify the interface is live.

Check that:

- WebSocket is connected;
- GNSS values continue updating;
- receiver status is not stale;
- Wi-Fi state matches the intended operating mode.

A loaded page with no live updates usually indicates a communication issue rather than a frontend rendering issue.

---

## Position & Velocity

The position section exposes the current receiver solution.

Use it to monitor:

- latitude
- longitude
- ellipsoidal height
- velocity
- PVT mode

The PVT mode is especially useful for distinguishing states such as:

```text
No PVT
StandAlone
DGNSS
RTK Float
RTK Fixed
```

The exact state depends on the receiver and current GNSS conditions.

---

## Satellites & Skyplot

The satellite view provides a visual representation of the current constellation geometry.

Use it to inspect:

- visible satellites;
- constellation distribution;
- general sky coverage;
- changing reception conditions.

The skyplot reflects data reported by the mosaic receiver.

---

## Quality & Receiver Status

WebUI exposes receiver-quality information in a compact form.

Use these indicators to identify:

- degraded receiver state;
- RF issues;
- unusual signal conditions;
- jamming-related warnings;
- receiver health changes.

For detailed receiver interpretation, use [GNSS Integration](../gnss/README.md) or Septentrio RxControl.

---

## Wi-Fi Configuration

WebUI can use:

- **AP mode** for local browser access;
- **STA mode** for external network / Internet access.

The local AP is:

```text
WEBUI_CONFIG
```

Use the Wi-Fi configuration panel to enter the external network credentials required for Station mode.

After connection, verify that an STA IP is shown before using Internet-dependent functions such as NTRIP.

---

## NTRIP Configuration

The NTRIP panel is used to configure correction services.

Typical fields are:

- caster host;
- port;
- username;
- password;
- mountpoint.

### Fetching mountpoints

Use **Fetch** to retrieve the caster sourcetable and populate the available streams.

A successful mountpoint fetch proves that the caster can be queried. It does not by itself prove that the selected correction stream is authenticated.

### Connecting

After selecting a valid mountpoint, request the NTRIP connection.

For a meaningful correction test, verify more than the `CONNECTED` state:

- valid GGA is available where required;
- RTCM traffic is active;
- the receiver reports correction use.

See [Connectivity](../connectivity/README.md).

---

## Bluetooth

When BLE is enabled, WebUI can expose the corresponding runtime state.

Use the interface to confirm whether BLE is:

- enabled;
- initialized;
- connected where applicable.

BLE is optional and does not need to be active for the browser interface to work.

---

## Expert Console

The **Expert Console** provides direct receiver interaction for advanced users.

Use it for:

- sending receiver commands;
- checking command responses;
- testing receiver configuration;
- low-level diagnostics.

> [!CAUTION]
> Commands sent from the Expert Console are forwarded to the receiver. Use only commands you understand, especially when changing persistent receiver configuration.

For routine monitoring, the normal dashboard should be preferred.

---

## Logging

WebUI can expose receiver logging controls when enabled by the firmware.

Use the logging section to:

- start logging;
- stop logging;
- verify logging state;
- observe related receiver feedback.

The exact logging destination and receiver behaviour depend on the current implementation and receiver configuration.

---

## Recommended Operating Sequence

For normal use:

1. connect to `WEBUI_CONFIG`;
2. open WebUI;
3. verify WebSocket and GNSS updates;
4. check the PVT solution;
5. enable STA if Internet access is needed;
6. configure NTRIP only after STA is online;
7. confirm RTCM activity if corrections are required.

This order avoids confusing a network problem with a receiver problem.

---

## WebUI vs. RxControl

WebUI is intended for embedded monitoring and field-oriented interaction.

Use **Septentrio RxControl** for:

- complete receiver configuration;
- firmware operations;
- advanced diagnostics;
- detailed receiver analysis.

WebUI has been cross-checked against RxControl for key receiver values, but it does not aim to reproduce every RxControl feature.

---

## If Something Looks Wrong

Use the dedicated troubleshooting guide for:

- missing WebUI assets;
- no GNSS data;
- stale values;
- Wi-Fi STA failures;
- mountpoint-fetch problems;
- NTRIP without RTCM;
- BLE issues.

See [Troubleshooting](../troubleshooting/README.md).

---

## Related Documentation

| Need | Guide |
|---|---|
| Install and launch WebUI | [Getting Started](../getting-started/README.md) |
| Understand GNSS values | [GNSS Integration](../gnss/README.md) |
| Configure Wi-Fi / NTRIP / BLE | [Connectivity](../connectivity/README.md) |
| Validate results | [Testing & Validation](../testing/README.md) |
| Diagnose issues | [Troubleshooting](../troubleshooting/README.md) |

---

<div align="center">

**Monitor first. Configure only what you need. Validate before trusting the result.**

[← Documentation Hub](../README.md)

</div>
