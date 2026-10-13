# Canon LBP-810 Driver for Modern macOS (Apple Silicon / ARM64)

An open-source, native CUPS driver for the **Canon LBP-810** laser printer on modern macOS (Sonoma, Sequoia, Tahoe / macOS 14, 15, 27) running on Apple Silicon (M1/M2/M3/M4).

[🇷🇺 Читать на русском (README.ru.md)](README.ru.md)

---

## Overview

The **Canon LBP-810** is a classic host-based (GDI) laser printer using the proprietary Canon **CAPT v1** protocol and **SCoA** (Smart Compression Architecture). Because Canon never released drivers for 64-bit macOS or Apple Silicon, this project provides a native solution:

* **Native macOS CUPS Backend:** Integrates directly into macOS printing architecture.
* **Apple Silicon Native (arm64):** Compiled with Clang for Apple Silicon, zero emulation required.
* **Self-Contained Binary:** Built with static `libusb`, no external Homebrew dependencies at runtime.
* **System Settings & GUI Integration:** Standard print dialog support across Preview, Safari, Pages, Microsoft Office, etc.
* **Clean Hardware Discovery:** Filters out broken default Apple USB entries from the "Add Printer" dialog.
* **Persistent Self-Healing Daemon:** Survives macOS system updates automatically.

---

## Architecture

```
[Application: Preview / Office / Safari]
                   │
                   ▼
         [macOS PrintCore]
                   │
                   ▼
       [CUPS Spooler (cupsd)]
                   │
                   ▼
 [cgpdftoraster] (Apple native PDF rasterizer)
                   │
                   ▼ (application/vnd.cups-raster, 600 DPI monochrome)
 [captusb] (CUPS Backend in /usr/libexec/cups/backend/captusb)
       │
       ├── High-speed SCoA line compression (libcapt)
       ├── CAPT v1 packet construction & handshaking
       ├── Exclusive USB interface control via libusb
       ├── Live status monitoring & error recovery (clear misprint)
       └── Real-time CUPS progress & state reporting (PAGE: N, STATE: ...)
                   │
                   ▼ (USB Bulk transfer)
         [Canon LBP-810 Printer]
```

---

## Installation

### Option 1: Graphical Installer (Recommended)
Open the disk image **`dist/Canon-LBP810-macOS.dmg`** and double-click **`Canon-LBP810-Driver-macOS.pkg`**.

### Option 2: Terminal Setup
Connect the printer to USB and run:
```bash
./setup.sh
```

The installer automatically:
1. Installs the backend into `/usr/libexec/cups/backend/captusb`.
2. Installs the PostScript Printer Description file (`CanonLBP-810.ppd`) into `/Library/Printers/PPDs/Contents/Resources/`.
3. Installs a LaunchDaemon (`com.canon.lbp810.heal`) that automatically restores the driver if a future macOS update wipes `/usr/libexec/cups/backend/`.
4. Detects the connected USB printer and creates the CUPS queue `Canon_LBP810`.

---

## Printing

Once installed, **Canon LBP-810** appears in **System Settings ▸ Printers & Scanners** and in standard print dialogs (`Cmd + P`).

From the terminal:
```bash
# Print a PDF document
lpr -P Canon_LBP810 document.pdf

# Check queue status
lpstat -p Canon_LBP810
```

---

## Known Issues (Version 1.0)

### Two "Canon LBP-810" printers displayed in macOS
On macOS (especially with Apple Silicon USB-C connections or local network printer sharing), **System Settings ▸ Printers & Scanners** and print dialogs (`Cmd + P`) may show two entries for **Canon LBP-810**:
1. **Working Driver Queue (`Canon_LBP810`):** Created by this driver, properly encodes CAPT v1 / SCoA raster data and transmits to hardware. Marked as *"Last Used"* (*Посл. исп.*).
2. **System Auto-Discovery Duplicate / Network Printer:** Created automatically by macOS USB plug-and-play or network Bonjour sharing. Sending jobs to this duplicate will fail with a transmission error ("Cannot send data to printer").

**Recommended Solution:**
* In **System Settings ▸ Printers & Scanners**, ensure **Default Printer** is set to **"Last Used"**.
* Select the working **Canon LBP-810** queue (marked as *"Last Used"* / *"Посл. исп."*) when printing. macOS will remember it and automatically default to the working queue for future jobs.

---

## Diagnostics

If the printer is not responding or you want to check hardware status:

```bash
./diagnose.sh
```

This utility checks:
- USB hardware detection and serial numbers.
- Backend binary installation and direct discovery.
- CUPS printer queue state.
- Recent entries in `/private/var/log/cups/error_log`.

---

## Uninstallation

To remove the printer queue, driver, and background daemons:

```bash
sudo ./uninstall.sh
```

---

## Building from Source & Running Tests

Prerequisites (only if building from source):
- CMake >= 3.21
- Xcode Command Line Tools (`xcode-select --install`)
- `libusb` and `pkg-config` (via Homebrew: `brew install libusb pkg-config`)

```bash
# Configure and build
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Run unit tests
ctest --test-dir build --output-on-failure
```

---

## Credits & Acknowledgements

* **[darkvision77/libcapt](https://github.com/darkvision77/libcapt)** and **[captppd](https://github.com/darkvision77/captppd)**: Reverse engineering of CAPT v1 and SCoA compression, CUPS backend base implementation.
* **[duy12i1i7/canon-LBP2900-for-macOS](https://github.com/duy12i1i7/canon-LBP2900-for-macOS)**: Modern macOS driver installation techniques and LaunchDaemon self-healing patterns.

## License

This project is licensed under the [BSD 2-Clause License](LICENSE).
