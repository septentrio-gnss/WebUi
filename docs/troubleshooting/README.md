# Troubleshooting

<div align="center">

<img src="../images/webui-logo.png" alt="WebUI Logo" width="220">

### Fast diagnosis for WebUI, GNSS, Wi-Fi and NTRIP issues

[← Documentation Hub](../README.md) · [Testing & Validation](../testing/README.md)

</div>

---

## Start Here

Diagnose from the lowest layer upward:

```text
Board boots
   ↓
WebUI loads
   ↓
UART receives data
   ↓
SBF / NMEA parses
   ↓
Wi-Fi STA works
   ↓
NTRIP / RTCM works
```

Do not troubleshoot NTRIP before the GNSS link is stable.

---

## WebUI Does Not Load

### `index.html.gz` not found

**Cause:** the firmware was uploaded, but the SPIFFS WebUI assets were not.

Run:

```bash
python3 compress_webui.py
pio run -e <environment> -t uploadfs
```

Windows PowerShell:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" .\compress_webui.py
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e <environment> -t uploadfs
```

---

### Old WebUI still appears

Force-refresh the browser:

```text
Ctrl + Shift + R
```

If needed, re-run `compress_webui.py` and `uploadfs`.

---

## Upload Fails

### Wrong boot mode / device will not enter download mode

Try:

1. hold **BOOT**;
2. press and release **RESET / EN**;
3. release **BOOT**;
4. retry the upload.

Also verify the selected serial port and PlatformIO environment.

---

## `WEBUI_CONFIG` Is Not Visible

Check:

- the ESP32 completed boot;
- the correct firmware environment was flashed;
- serial logs show AP initialization;
- the board is not repeatedly resetting.

Default access:

```text
SSID: WEBUI_CONFIG
IP:   192.168.3.1
```

---

## No GNSS Data

Use the runtime counters first.

| Observation | Check |
|---|---|
| `bytes/s = 0` | wiring, GND, UART pins, baud rate, receiver power |
| bytes > 0, `SBF sync/s = 0` | receiver output format / wrong COM stream |
| SBF sync > 0, parsed = 0 | expected blocks / parser compatibility |
| `NMEA/s = 0` | NMEA stream not enabled |
| `GGA/s = 0` | GGA stream not enabled |

Correct UART wiring:

```text
mosaic TX → ESP32 RX
mosaic RX ← ESP32 TX
GND       ↔ GND
```

Remember:

> ESP32 `Serial2` does not imply mosaic `COM2`.

The receiver stream must target the physical mosaic port actually connected to the ESP32.

---

## WebUI Works but GNSS Values Do Not Update

Check that:

- SBF blocks are being parsed repeatedly;
- the receiver has a valid PVT solution;
- WebSocket is connected;
- the browser is receiving fresh status updates.

A parsed `PVTGeodetic` block does not automatically mean the position is valid.

---

## Wi-Fi STA Does Not Connect

Check:

- SSID and password;
- the network is compatible with 2.4 GHz Wi-Fi;
- the ESP32 receives an STA IP;
- credentials were saved correctly.

The local AP may still work even when STA fails.

---

## Mountpoints Cannot Be Fetched

Verify in this order:

1. STA is connected.
2. ESP32 has Internet access.
3. caster host is correct.
4. caster port is correct.
5. the caster returns a sourcetable.

If the browser remains stuck on **Fetching...**, inspect WebSocket request/response logs.

---

## NTRIP Says Connected but No Corrections Arrive

Do not rely only on socket state.

Verify:

```text
Session accepted?
      ↓
GGA available?
      ↓
RTCM bytes received?
      ↓
RTCM forwarded?
      ↓
Receiver uses corrections?
```

A valid correction test should show actual RTCM activity and, where conditions allow, a DGNSS / RTK-related receiver state.

---

## NTRIP Disconnects Repeatedly

Check:

- Internet stability;
- mountpoint validity;
- credentials;
- GGA availability for VRS services;
- caster response;
- reconnect interval.

A VRS stream may close if the caster does not receive valid rover position information.

---

## Repeated I²C Errors on a Generic DevKit

Typical symptom:

```text
I2C write failed
addr=0x30 / 0x31
```

If the board has no KTD2026 LED drivers, disable them in the board profile:

```ini
-DWEBUI_HAS_KTD2026=0
```

Do not treat absent optional hardware as a GNSS failure.

---

## BLE Is Not Visible

Check:

- BLE is enabled;
- initialization completed;
- the expected BLE name is configured;
- no existing client is holding the connection;
- the selected board profile supports the feature.

---

## NVS `NOT_FOUND`

A `NOT_FOUND` message during first boot can simply mean a configuration key has not been stored yet.

It becomes a problem only if:

- required configuration never saves;
- values disappear after reboot;
- the application repeatedly fails because the key is missing.

---

## Fast Recovery Checklist

When the cause is unclear:

- [ ] reboot the ESP32;
- [ ] open the serial monitor;
- [ ] confirm SPIFFS mounted;
- [ ] confirm `WEBUI_CONFIG`;
- [ ] confirm WebSocket connection;
- [ ] check GNSS counters;
- [ ] confirm the correct mosaic COM port;
- [ ] confirm STA Internet access;
- [ ] inspect NTRIP response and RTCM activity.

---

## Related Guides

| Issue | Guide |
|---|---|
| Build / upload | [Getting Started](../getting-started/README.md) |
| UART / board profile | [Hardware](../hardware/README.md) |
| SBF / NMEA | [GNSS Integration](../gnss/README.md) |
| Wi-Fi / NTRIP / BLE | [Connectivity](../connectivity/README.md) |
| Expected test result | [Testing & Validation](../testing/README.md) |

---

<div align="center">

**Troubleshoot from transport → protocol → application → external service.**

[← Documentation Hub](../README.md)

</div>
