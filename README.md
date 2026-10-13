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
* **Live Menu Bar Progress:** Native status app displays real-time page progress (`🖨 2/5`) and job notifications in the menu bar.
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
