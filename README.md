# UnoFace

[![Build and Release](https://github.com/pedro/UnoFace/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/pedro/UnoFace/actions/workflows/build-and-release.yml)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011-blue.svg)](https://microsoft.com)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17%20%2F%20C%2B%2B20-orange.svg)](https://isocpp.org)
[![Target](https://img.shields.io/badge/Hardware-UGREEN%20Uno%206--in--1%20(35998)-success.svg)](https://www.ugreen.com)

**C++ native Windows GUI application** built for hardware telemetry, real-time status mirroring, and reverse-engineering investigation of the **UGREEN Uno USB-C 6-in-1 Hub (Model 35998 / ASIN B0D2Q5XJY9)**.

---

## Table of Contents

1. [Hardware Overview](#hardware-overview)
2. [Official Firmware Faces](#official-firmware-faces)
3. [Key Findings from USB Bus Analysis](#key-findings-from-usb-bus-analysis)
4. [Software Architecture](#software-architecture)
5. [User Interface & Capabilities](#user-interface--capabilities)
6. [Automated CI/CD & GitHub Releases](#automated-cicd--github-releases)
7. [Building from Source](#building-from-source)
8. [Automated Unit Testing](#automated-unit-testing)

---

## Hardware Overview

The **UGREEN Uno USB-C 6-in-1 Hub** provides 10 Gbps USB 3.2 Gen 2 data ports, 4K@60Hz HDMI video output, and 100W USB-C Power Delivery (PD) pass-through charging, alongside an integrated front screen with robotic status emojis.

During physical bus enumeration under Windows, the device exposes two standard Genesys Logic controller instances:

| Device | VID / PID | Class | Role |
| --- | --- | --- | --- |
| **GL3523 Companion Hub** | `0x05E3:0x0610` | `0x09` (Hub) | Manages USB 2.1 downstream ports and peripherals |
| **GL3523 SuperSpeed Hub** | `0x05E3:0x0625` | `0x09` (Hub) | Manages USB 3.2 Gen 2 SuperSpeed data links (10 Gbps) |

---

## Official Firmware Faces

| Face (ASCII) | State |
|---|---|
| `-     -` | Connect to Host |
| `-     -`<br>`   <` | Connect to External Display |
| `^     ^` | Connect to PD Charger |
| `^     ^`<br>`   <` | External Display + PD Charging |

---

## Key Findings from USB Bus Analysis

During our live capture sessions with `UnoFace`, physical disconnection and reconnection of display cables revealed critical architectural facts:

1. **Autonomous Internal Display Controller:**
   The front screen reacts to physical cable connections on the hub itself rather than host software commands.
   - **Mouth (`<`)**: Triggered when an active display link is connected to the hub's HDMI port.
   - **Eyes (`^   ^`)**: Triggered when a charger is plugged into the hub's USB-C Power Delivery port.

2. **No Host-Facing Display Interface:**
   The hub enumerates strictly with standard USB Hub class interfaces (`bInterfaceClass = 0x09`). It does not expose any HID, CDC-ACM (Serial), or Vendor-defined USB endpoints to the host operating system. Furthermore, no USB Billboard capability (`0x0D`) is advertised in the BOS descriptor.

3. **Separation of Hub and Host Native Ports:**
   Operating systems track global power and monitor states across the entire machine. If a laptop is plugged into its own wall charger or an external monitor is connected to a native motherboard/GPU HDMI port, global OS APIs reflect host power and video without those signals traversing the USB hub. `UnoFace` explicitly tracks cables connected to the hub itself to prevent false positives.

---

## Software Architecture

`UnoFace` follows clean, layered architecture principles with zero external UI framework dependencies (pure native Win32 API):

```text
include/unoface/
├── core/
│   ├── expression.hpp          # Official Expression enum, display names, and ASCII art
│   ├── host_signals.hpp        # HubAttachedCables struct and face inference logic
│   ├── snapshot.hpp            # Snapshot state and differential change engine
│   └── types.hpp               # GL3523 hardware identifiers and USB descriptors
├── usb/
│   ├── descriptor_parser.hpp   # Robust binary descriptor parser (Config, BOS, String)
│   ├── ihub_enumerator.hpp     # Abstract hub enumeration interface
│   └── win32_hub_enumerator.hpp# Native Windows IOCTL and SetupAPI implementation
├── platform/
│   └── win32_system.hpp        # High-resolution clock, directory, and ShellExecute helpers
├── app/
│   └── hub_monitor.hpp         # Hub monitoring and snapshot capture coordinator
├── investigation/
│   └── capture_recorder.hpp    # Automated event logger with disk persistence
└── gui/
    └── main_window.hpp         # Native Win32 GUI window and message loop
```

---

## User Interface & Capabilities

- **Stand-alone Native Executable**: Built with `-mwindows` / `WIN32` subsystem. Double-clicking `unoface.exe` immediately opens a clean GUI window with no flashing console window.
- **OLED Display Mirror**: Pixel-perfect simulation box rendered in deep OLED black (`#12141C`) with glowing cyan characters (`#00F0D2`) using a 28pt Consolas font.
- **Accurate Physical Cable Toggles**: Dedicated checkboxes represent what is physically attached to the UGREEN Hub:
  - `[ ] HDMI Monitor plugged into UGREEN Hub`
  - `[ ] USB-C PD Charger plugged into UGREEN Hub`
- **Real-Time USB Hardware Discovery**: Continuously detects GL3523 companion hubs (`05E3:0610` and `05E3:0625`), querying firmware BCD revisions and active downstream ports.
- **Investigation Session Recorder**:
  - **Start Recording**: Generates a timestamped capture file in `captures/capture-YYYYMMDD-HHMMSS.log`.
  - **Live Diff Detection**: Automatically logs every port and cable transition triggered by Windows device change notifications (`WM_DEVICECHANGE`).
  - **User Notes**: Allows annotating observations during physical tests directly into the log stream.
  - **Open Captures Folder**: One-click shortcut to inspect log files in Windows Explorer.

---

## Automated CI/CD & GitHub Releases

The repository includes a production-ready GitHub Actions workflow ([`.github/workflows/build-and-release.yml`](.github/workflows/build-and-release.yml)):

- **Continuous Integration**: Triggers on every push and pull request to `main`.
- **Automated Matrix Build**: Compiles `unoface.exe` on `windows-latest` using CMake and Microsoft Visual C++ (MSVC).
- **Automated Test Run**: Executes the 24-test suite with `ctest`.
- **Artifact Publishing**: Uploads the compiled standalone binary as `UnoFace-Windows-x64` for every run.
- **Automated Releases**: When a Git tag matching `v*` (e.g. `v0.2.0`) is pushed, the workflow automatically generates a formal GitHub Release and attaches `unoface.exe`.

### Releasing a New Version

```bash
git tag v0.2.0
git push origin v0.2.0
```

---

## Building from Source

### Prerequisites
- Windows 10 or 11
- C++17 or C++20 compatible compiler:
  - **MinGW-w64** (`g++`) **OR**
  - **Microsoft Visual Studio 2019/2022** (MSVC)

### Build via PowerShell (MinGW)
```powershell
.\build.ps1
```
The resulting standalone binaries will be placed in `build/`:
- `build/unoface.exe` — Standalone GUI application
- `build/test_unoface.exe` — Unit test executable

### Build via CMake (MSVC / Visual Studio)
```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## Automated Unit Testing

The test suite validates domain logic, descriptor decoding, and capture diffing without requiring connected hardware:

```powershell
.\build\test_unoface.exe
```

Expected output:
```text
=== Running UnoFace Tests ===
[PASS] Four official expressions registered
[PASS] Nothing attached to hub -> HostConnected
[PASS] Hub HDMI attached -> DisplayConnected
[PASS] Hub PD attached -> PdConnected
[PASS] Both attached -> DisplayAndPdConnected
[PASS] HostConnected ascii art has eyes
[PASS] DisplayConnected ascii art has mouth
[PASS] Diff detected changes
[PASS] Diff tracks hub PD charger change
[PASS] Diff tracks port connection change
[PASS] read_total_length parses total length
[PASS] parse_interfaces found 1 interface
[PASS] Interface number is 0
[PASS] Interface class is 0x09 (Hub)
[PASS] parse_bos_capabilities found 2 capabilities
[PASS] First cap is USB 2.0 Extension
[PASS] Second cap is SuperSpeed USB
[PASS] parse_string_descriptor parses UTF-16LE string
[PASS] Recorder is active
[PASS] Start emitted baseline lines
[PASS] Note written
[PASS] Observed cable change
[PASS] Recorder stopped
[PASS] Stop emitted end marker
=== All tests passed! ===
```

---

## License

This project is licensed under the MIT License. See `LICENSE` for details.
