#!/bin/bash
#
# Canon LBP-810 Driver Installer for macOS (Apple Silicon / ARM64)
# Usage: sudo ./install.sh
#
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BACKEND_SRC="$DIR/prebuilt/arm64/captusb"
PPD_SRC="$DIR/prebuilt/arm64/LBP810CAPTPPD.ppd"

BACKEND_DST="/usr/libexec/cups/backend/captusb"
PPD_DIR="/Library/Printers/PPDs/Contents/Resources"
PPD_DST="$PPD_DIR/CanonLBP-810.ppd"

QUEUE_NAME="Canon_LBP810"
QUEUE_DESC="Canon LBP-810"

STASH="/Library/Application Support/CanonLBP810"
HEAL_SCRIPT="$STASH/heal.sh"
DAEMON_LABEL="com.canon.lbp810.heal"
DAEMON_PLIST="/Library/LaunchDaemons/$DAEMON_LABEL.plist"

LPINFO="/usr/sbin/lpinfo"
LPADMIN="/usr/sbin/lpadmin"
CUPSENABLE="/usr/sbin/cupsenable"
CUPSACCEPT="/usr/sbin/cupsaccept"

if [ "$(id -u)" -ne 0 ]; then
  echo "Error: please run as root: sudo ./install.sh" >&2
  exit 1
fi

echo "=========================================================="
echo "  Installing Canon LBP-810 Driver for macOS (ARM64)"
echo "=========================================================="

# Build if prebuilt missing
if [ ! -f "$BACKEND_SRC" ] || [ ! -f "$PPD_SRC" ]; then
  echo "==> Prebuilt binaries not found, building from source via CMake..."
  (
    cd "$DIR"
    cmake -B build -DBUILD_TESTS=OFF
    cmake --build build
    mkdir -p "$DIR/prebuilt/arm64"
    cp build/captusb "$DIR/prebuilt/arm64/"
    cp build/ppd/LBP810CAPTPPD.ppd "$DIR/prebuilt/arm64/"
  )
  BACKEND_SRC="$DIR/prebuilt/arm64/captusb"
  PPD_SRC="$DIR/prebuilt/arm64/LBP810CAPTPPD.ppd"
fi

# 1. Install Backend
echo "==> [1/5] Installing CUPS backend -> $BACKEND_DST"
install -o root -g wheel -m 0755 "$BACKEND_SRC" "$BACKEND_DST"

# 2. Install PPD
echo "==> [2/5] Installing PPD file     -> $PPD_DST"
mkdir -p "$PPD_DIR"
install -o root -g wheel -m 0644 "$PPD_SRC" "$PPD_DST"

# Also symlink to CUPS model directory if accessible
mkdir -p /usr/share/cups/model 2>/dev/null || true
if [ -d /usr/share/cups/model ]; then
  cp "$PPD_SRC" /usr/share/cups/model/LBP810CAPTPPD.ppd 2>/dev/null || true
fi

# 3. Setup Persistent Self-Healing Daemon & Helpers
echo "==> [3/5] Setting up self-healing LaunchDaemon -> $DAEMON_LABEL"
mkdir -p "$STASH"
install -o root -g wheel -m 0755 "$BACKEND_SRC" "$STASH/captusb"
install -o root -g wheel -m 0755 "$DIR/prebuilt/arm64/usb_filter" "$STASH/usb_filter"
install -o root -g wheel -m 0644 "$PPD_SRC" "$STASH/CanonLBP-810.ppd"

# Install USB backend filter to hide broken Apple USB from Add Printer dialog
if [ -f /usr/libexec/cups/backend/usb ] && [ ! -f /usr/libexec/cups/backend/usb.orig ]; then
  echo "    Filtering Apple USB backend discovery for LBP-810..."
  cp /usr/libexec/cups/backend/usb /usr/libexec/cups/backend/usb.orig
  install -o root -g wheel -m 0755 "$DIR/prebuilt/arm64/usb_filter" /usr/libexec/cups/backend/usb
fi

cat > "$HEAL_SCRIPT" << 'HEALEOF'
#!/bin/bash
# Auto-restore Canon LBP-810 backend and PPD after macOS system updates
STASH="/Library/Application Support/CanonLBP810"
BACKEND="/usr/libexec/cups/backend/captusb"
PPD_DIR="/Library/Printers/PPDs/Contents/Resources"
PPD="$PPD_DIR/CanonLBP-810.ppd"
QUEUE="Canon_LBP810"

if [ ! -x "$BACKEND" ] && [ -f "$STASH/captusb" ]; then
  /usr/bin/install -o root -g wheel -m 0755 "$STASH/captusb" "$BACKEND"
  /usr/bin/logger -t lbp810-heal "Restored CUPS backend captusb"
fi

if [ -f "$STASH/usb_filter" ] && [ ! -f /usr/libexec/cups/backend/usb.orig ]; then
  /bin/cp /usr/libexec/cups/backend/usb /usr/libexec/cups/backend/usb.orig 2>/dev/null || true
  /usr/bin/install -o root -g wheel -m 0755 "$STASH/usb_filter" /usr/libexec/cups/backend/usb 2>/dev/null || true
fi

if [ ! -f "$PPD" ] && [ -f "$STASH/CanonLBP-810.ppd" ]; then
  /bin/mkdir -p "$PPD_DIR"
  /usr/bin/install -o root -g wheel -m 0644 "$STASH/CanonLBP-810.ppd" "$PPD"
  /usr/bin/logger -t lbp810-heal "Restored PPD file"
fi

URI="$("$BACKEND" 2>/dev/null | /usr/bin/awk '$1 == "direct" && $2 ~ /^captusb:/ {print $2; exit}' || true)"
if [ -z "$URI" ]; then
  URI="captusb://Canon/LBP-810?drv=capt"
fi

if ! /usr/bin/lpstat -p "$QUEUE" >/dev/null 2>&1; then
  if [ -f "$PPD" ]; then
    /usr/sbin/lpadmin -x Canon_LBP_810 2>/dev/null || true
    /usr/sbin/lpadmin -x "Canon_LBP-810" 2>/dev/null || true
    /usr/sbin/lpadmin -p "$QUEUE" -v "$URI" -P "$PPD" -E -D "Canon LBP-810" \
      -o printer-is-shared=false -o printer-error-policy=retry-job
    /usr/sbin/lpadmin -d "$QUEUE" 2>/dev/null || true
    /usr/sbin/cupsenable "$QUEUE" 2>/dev/null || true
    /usr/sbin/cupsaccept "$QUEUE" 2>/dev/null || true
    /usr/bin/logger -t lbp810-heal "Created print queue $QUEUE"
  fi
fi
exit 0
HEALEOF
chmod 0755 "$HEAL_SCRIPT"

cat > "$DAEMON_PLIST" << PLISTEOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>$DAEMON_LABEL</string>
    <key>ProgramArguments</key>
    <array>
        <string>/bin/bash</string>
        <string>$HEAL_SCRIPT</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>StartInterval</key>
    <integer>15</integer>
</dict>
</plist>
PLISTEOF
chown root:wheel "$DAEMON_PLIST"
chmod 0644 "$DAEMON_PLIST"
launchctl unload "$DAEMON_PLIST" 2>/dev/null || true
launchctl load "$DAEMON_PLIST" 2>/dev/null || true

# Clean up any legacy menubar progress app from previous installs
launchctl unload /Library/LaunchAgents/com.canon.lbp810.progress.plist 2>/dev/null || true
rm -f /Library/LaunchAgents/com.canon.lbp810.progress.plist 2>/dev/null || true
rm -rf "$STASH/CanonLBP810Progress.app" 2>/dev/null || true
killall CanonLBP810Progress 2>/dev/null || true

# 4. Search for USB printer & Configure Queue
echo "==> [4/5] Configuring CUPS print queue '$QUEUE_NAME'..."
DEVICE_URI="$("$BACKEND_DST" 2>/dev/null | awk '$1 == "direct" && $2 ~ /^captusb:/ {print $2; exit}' || true)"

if [ -z "$DEVICE_URI" ]; then
  # Fallback: check lpinfo
  DEVICE_URI="$("$LPINFO" -v 2>/dev/null | awk '$2 ~ /^captusb:/ {print $2; exit}' || true)"
fi

if [ -z "$DEVICE_URI" ]; then
  echo "    Printer not connected yet; using universal hardware URI."
  DEVICE_URI="captusb://Canon/LBP-810?drv=capt"
else
  echo "    Live printer found: $DEVICE_URI"
fi

# Remove conflicting legacy / auto-added Apple USB queues
"$LPADMIN" -x Canon_LBP_810 2>/dev/null || true
"$LPADMIN" -x "Canon_LBP-810" 2>/dev/null || true

echo "==> [5/5] Creating CUPS print queue '$QUEUE_NAME'..."
"$LPADMIN" -p "$QUEUE_NAME" -v "$DEVICE_URI" -P "$PPD_DST" -E -D "$QUEUE_DESC" \
  -o printer-is-shared=false -o printer-error-policy=retry-job
"$LPADMIN" -d "$QUEUE_NAME"
"$CUPSENABLE" "$QUEUE_NAME" 2>/dev/null || true
"$CUPSACCEPT" "$QUEUE_NAME" 2>/dev/null || true
echo "    Print queue successfully created and set as system default!"

echo "=========================================================="
echo "  ✅ Installation complete!"
echo "  Printer is ready to use in System Settings / Print dialogs."
echo "=========================================================="
