# Getting Started

<div align="center">

<img src="../images/webui-logo.png" alt="WebUI Logo" width="220">

### Build, flash and launch WebUI on a supported ESP32-S3 platform

[← Documentation Hub](../README.md) · [Hardware & Board Profiles](../hardware/README.md) · [Troubleshooting](../troubleshooting/README.md)

</div>

---

## Overview

This guide takes you from a fresh clone of the repository to a running **WebUI** instance accessible from a browser.

The standard deployment consists of two separate uploads:

1. the **ESP32 firmware**
2. the **WebUI filesystem assets** stored in SPIFFS

Both are required for a complete installation.

> [!IMPORTANT]
> Uploading the firmware does **not** automatically upload the WebUI files.  
> If the firmware boots but the browser reports that `index.html` cannot be found, upload the SPIFFS filesystem as described below.

---

## Before You Start

You will need:

- a compatible **ESP32-S3** board
- a **Septentrio mosaic** GNSS receiver for full GNSS functionality
- a USB data cable
- a computer running Windows, Linux or macOS
- Git
- PlatformIO
- a modern web browser

For UART wiring, supported board profiles and hardware-specific configuration, see the [Hardware & Board Profiles Guide](../hardware/README.md).

---

## 1. Install the Development Tools

### Recommended: Visual Studio Code + PlatformIO

Install:

- [Visual Studio Code](https://code.visualstudio.com/)
- the **PlatformIO IDE** extension

PlatformIO manages the ESP32 toolchain, libraries, build system and upload tools used by this project.

### PlatformIO CLI

If PlatformIO is already installed, verify it with:

```bash
pio --version
```

On Windows, if `pio` is not available in `PATH`, the PlatformIO executable installed by the VS Code extension can normally be called directly from PowerShell:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" --version
```

---

## 2. Clone the Repository

<table>
<tr>
<th width="50%">🐧 Linux / macOS</th>
<th width="50%">🪟 Windows PowerShell</th>
</tr>
<tr>
<td valign="top">

<pre><code>git clone https://github.com/septentrio-gnss/WebUi.git
cd WebUi</code></pre>

</td>
<td valign="top">

<pre><code>git clone https://github.com/septentrio-gnss/WebUi.git
cd WebUi</code></pre>

</td>
</tr>
</table>

The project root should contain at least:

```text
WebUi/
├── src/
├── include/
├── data_src/
├── data/
├── docs/
├── compress_webui.py
├── platformio.ini
└── README.md
```

---

## 3. Select the PlatformIO Environment

WebUI can contain multiple PlatformIO environments for different ESP32-S3 hardware profiles.

The available environments are defined in:

[`platformio.ini`](../../platformio.ini)

Open that file and identify the environment matching your board.

An environment appears as:

```ini
[env:environment-name]
```

Use that name in the commands below:

```text
<environment>
```

For example:

```bash
pio run -e <environment>
```

> [!NOTE]
> Flash size, PSRAM mode, USB configuration, UART pins and optional peripherals may differ between boards. Do not assume that one environment is valid for every ESP32-S3 target.

See [Hardware & Board Profiles](../hardware/README.md) before adapting the firmware to a new board.

---

## 4. Connect the Hardware

### ESP32

Connect the ESP32-S3 to the computer using a **USB data cable**.

### GNSS receiver

For full operation, connect the ESP32 UART to the Septentrio mosaic receiver:

```text
mosaic TX  ─────→  ESP32 RX
mosaic RX  ←─────  ESP32 TX
GND        ──────  GND
```

The exact UART pins are board-profile dependent.

> [!CAUTION]
> Verify voltage levels and board-specific wiring before connecting hardware.  
> Do not connect an external power pin unless the hardware profile explicitly requires it.

See the [Hardware & Board Profiles Guide](../hardware/README.md) for the exact wiring model.

---

## 5. Build the Firmware

<table>
<tr>
<th width="50%">🐧 Linux / macOS</th>
<th width="50%">🪟 Windows PowerShell</th>
</tr>
<tr>
<td valign="top">

<pre><code>pio run -e &lt;environment&gt;</code></pre>

</td>
<td valign="top">

<pre><code>& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e &lt;environment&gt;</code></pre>

</td>
</tr>
</table>

A successful build should end without compilation errors.

---

## 6. Prepare the WebUI Assets

The editable frontend lives in `data_src/`.

Before uploading it to the ESP32, run the compression script from the **repository root**.

<table>
<tr>
<th width="50%">🐧 Linux / macOS</th>
<th width="50%">🪟 Windows PowerShell</th>
</tr>
<tr>
<td valign="top">

<pre><code>python3 compress_webui.py</code></pre>

</td>
<td valign="top">

<pre><code>& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" .\compress_webui.py</code></pre>

</td>
</tr>
</table>

The generated files are placed in `data/` and are then uploaded to SPIFFS.

Typical generated assets include:

```text
data/
├── index.html.gz
├── style.css.gz
├── chart.js.gz
└── ...
```

> [!TIP]
> On Windows, using PlatformIO's bundled Python avoids problems when Python is not installed globally or is not available in `PATH`.

---

## 7. Upload the Firmware

<table>
<tr>
<th width="50%">🐧 Linux / macOS</th>
<th width="50%">🪟 Windows PowerShell</th>
</tr>
<tr>
<td valign="top">

<pre><code>pio run -e &lt;environment&gt; -t upload</code></pre>

</td>
<td valign="top">

<pre><code>& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e &lt;environment&gt; -t upload</code></pre>

</td>
</tr>
</table>

If the correct serial port is not selected automatically, specify it in the corresponding PlatformIO environment or use the appropriate upload-port option.

---

## 8. Upload the WebUI Filesystem

After the firmware upload, upload the SPIFFS image:

<table>
<tr>
<th width="50%">🐧 Linux / macOS</th>
<th width="50%">🪟 Windows PowerShell</th>
</tr>
<tr>
<td valign="top">

<pre><code>pio run -e &lt;environment&gt; -t uploadfs</code></pre>

</td>
<td valign="top">

<pre><code>& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e &lt;environment&gt; -t uploadfs</code></pre>

</td>
</tr>
</table>

A successful filesystem upload means the compressed WebUI assets are now stored on the ESP32.

### Firmware vs. Filesystem

| Change | Firmware upload | `uploadfs` |
|---|:---:|:---:|
| C++ source code | ✅ | — |
| Board configuration | ✅ | — |
| GNSS / NTRIP logic | ✅ | — |
| HTML | — | ✅ |
| CSS | — | ✅ |
| JavaScript | — | ✅ |
| Images / Web assets | — | ✅ |

For frontend-only changes, you normally only need:

```text
Edit data_src/
      ↓
Run compress_webui.py
      ↓
Upload filesystem
```

---

## 9. Open the Serial Monitor

The serial monitor is the fastest way to verify the boot sequence and diagnose configuration issues.

<table>
<tr>
<th width="50%">🐧 Linux / macOS</th>
<th width="50%">🪟 Windows PowerShell</th>
</tr>
<tr>
<td valign="top">

<pre><code>pio device monitor -e &lt;environment&gt;</code></pre>

</td>
<td valign="top">

<pre><code>& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor -e &lt;environment&gt;</code></pre>

</td>
</tr>
</table>

The configured monitor baud rate is defined in `platformio.ini`.

During a normal boot, you should see the ESP32 initialize the filesystem, networking services, WebUI server and GNSS communication.

---

## 10. Connect to WebUI

After startup, WebUI creates a local Wi-Fi Access Point.

| Setting | Default |
|---|---|
| **SSID** | `WEBUI_CONFIG` |
| **IP address** | `192.168.3.1` |
| **mDNS** | `http://webui.local` |

Connect your laptop, smartphone or tablet to:

```text
WEBUI_CONFIG
```

Then open:

```text
http://192.168.3.1
```

or, where mDNS is supported:

```text
http://webui.local
```

> [!NOTE]
> A device connected only to `WEBUI_CONFIG` can access the local WebUI without Internet access.  
> Internet-dependent features such as NTRIP require the ESP32 to also connect to an external Wi-Fi network in Station mode.

---

## 11. First-Run Validation

Once the page loads, verify the installation in this order:

### WebUI

- the dashboard loads without missing assets
- WebSocket status becomes active
- live status information updates

### GNSS link

With a mosaic receiver connected:

- UART traffic is detected
- SBF and/or NMEA messages are parsed
- receiver information becomes available
- GNSS fields update in the interface

### Wi-Fi

- `WEBUI_CONFIG` remains accessible
- Station mode can connect to an external 2.4 GHz Wi-Fi network when configured

### NTRIP

For correction testing:

- Station Wi-Fi has Internet access
- caster configuration is valid
- mountpoints can be retrieved
- a valid GGA is available when required
- RTCM data is received and forwarded to the receiver

See:

- [GNSS Integration](../gnss/README.md)
- [Connectivity](../connectivity/README.md)
- [Testing & Validation](../testing/README.md)

---

## Common First-Boot Problems

### WebUI page is missing

Typical serial output may indicate that `index.html.gz` or `index.html` is not present.

Run:

```text
compress_webui.py
      ↓
uploadfs
```

The firmware and WebUI filesystem are uploaded separately.

---

### ESP32 is not detected

Check:

- the USB cable supports data
- the correct USB connector is being used
- the board appears in the operating system
- the required USB-to-UART driver is installed, if the board uses one

Some boards use native ESP32-S3 USB, while others expose a USB-to-UART bridge such as CP210x.

---

### Upload fails with a boot-mode error

If the ESP32 does not automatically enter download mode:

1. hold **BOOT**
2. press and release **RESET / EN**
3. release **BOOT**
4. retry the upload

Exact behavior depends on the board.

---

### WebUI still shows old frontend content

After uploading new WebUI assets, force-refresh the browser:

```text
Ctrl + Shift + R
```

Browser caching can otherwise make an older interface appear to still be installed.

---

### No GNSS data appears

Check the complete UART path:

```text
mosaic TX → ESP32 RX
mosaic RX ← ESP32 TX
GND       ↔ GND
```

Also verify:

- selected UART pins
- receiver baud rate
- selected receiver COM port
- required SBF / NMEA streams
- selected PlatformIO board profile

See [GNSS Integration](../gnss/README.md) and [Troubleshooting](../troubleshooting/README.md).

---

## Updating an Existing Installation

### Firmware-only change

```text
Edit C++ code
     ↓
Build
     ↓
Upload firmware
```

### WebUI-only change

```text
Edit data_src/
     ↓
compress_webui.py
     ↓
uploadfs
```

### Firmware + WebUI change

```text
Build firmware
     ↓
compress_webui.py
     ↓
Upload firmware
     ↓
Upload filesystem
```

---

## Next Steps

Once WebUI is running:

| Goal | Continue with |
|---|---|
| Learn the interface | [User Guide](../user-guide/README.md) |
| Understand wiring and board profiles | [Hardware & Board Profiles](../hardware/README.md) |
| Understand the firmware internals | [Architecture](../architecture/README.md) |
| Configure Wi-Fi, NTRIP or BLE | [Connectivity](../connectivity/README.md) |
| Understand SBF / NMEA integration | [GNSS Integration](../gnss/README.md) |
| Validate the installation | [Testing & Validation](../testing/README.md) |
| Diagnose a problem | [Troubleshooting](../troubleshooting/README.md) |

---

<div align="center">

### WebUI Getting Started

**Clone → Configure → Build → Flash → Connect**

[← Documentation Hub](../README.md) · [Next: Hardware & Board Profiles →](../hardware/README.md)

</div>
