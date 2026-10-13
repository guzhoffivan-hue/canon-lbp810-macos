#!/bin/bash
#
# Script to build Apple Installer Package (.pkg) for Canon LBP-810
#
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$DIR/build_pkg"
PKG_ROOT="$BUILD_DIR/root"
SCRIPTS_DIR="$BUILD_DIR/scripts"
OUTPUT_DIR="$DIR/dist"
OUTPUT_PKG="$OUTPUT_DIR/Canon-LBP810-Driver-macOS.pkg"

echo "=========================================================="
echo "  Building macOS Installer Package (.pkg)"
echo "=========================================================="

rm -rf "$BUILD_DIR"
mkdir -p "$PKG_ROOT" "$SCRIPTS_DIR" "$OUTPUT_DIR"

# 1. Compile binaries if needed
(
  cd "$DIR"
  cmake -B build -DBUILD_TESTS=OFF
  cmake --build build
  cp build/captusb prebuilt/arm64/
  cp build/usb_filter prebuilt/arm64/
  cp build/ppd/LBP810CAPTPPD.ppd prebuilt/arm64/
  swiftc -O -target arm64-apple-macos14.0 src/menubar/CanonLBP810Progress.swift -o prebuilt/arm64/CanonLBP810Progress
  mkdir -p prebuilt/arm64/CanonLBP810Progress.app/Contents/MacOS
  cp prebuilt/arm64/CanonLBP810Progress prebuilt/arm64/CanonLBP810Progress.app/Contents/MacOS/
)

# 2. Stage files in root
STASH_DIR="$PKG_ROOT/Library/Application Support/CanonLBP810"
mkdir -p "$PKG_ROOT/usr/libexec/cups/backend"
mkdir -p "$PKG_ROOT/Library/Printers/PPDs/Contents/Resources"
mkdir -p "$PKG_ROOT/Library/LaunchDaemons"
mkdir -p "$PKG_ROOT/Library/LaunchAgents"
mkdir -p "$STASH_DIR"

cp "$DIR/prebuilt/arm64/captusb" "$PKG_ROOT/usr/libexec/cups/backend/captusb"
cp "$DIR/prebuilt/arm64/LBP810CAPTPPD.ppd" "$PKG_ROOT/Library/Printers/PPDs/Contents/Resources/CanonLBP-810.ppd"

# Stash for self-healing
cp "$DIR/prebuilt/arm64/captusb" "$STASH_DIR/captusb"
cp "$DIR/prebuilt/arm64/usb_filter" "$STASH_DIR/usb_filter"
cp "$DIR/prebuilt/arm64/LBP810CAPTPPD.ppd" "$STASH_DIR/CanonLBP-810.ppd"
cp -r "$DIR/prebuilt/arm64/CanonLBP810Progress.app" "$STASH_DIR/"

# Heal script
cat > "$STASH_DIR/heal.sh" << 'HEALEOF'
#!/bin/bash
STASH="/Library/Application Support/CanonLBP810"
BACKEND="/usr/libexec/cups/backend/captusb"
USB_BACKEND="/usr/libexec/cups/backend/usb"
PPD_DIR="/Library/Printers/PPDs/Contents/Resources"
PPD="$PPD_DIR/CanonLBP-810.ppd"
QUEUE="Canon_LBP810"

if [ ! -x "$BACKEND" ] && [ -f "$STASH/captusb" ]; then
  /usr/bin/install -o root -g wheel -m 0700 "$STASH/captusb" "$BACKEND"
fi

if [ -f "$STASH/usb_filter" ] && [ ! -f /usr/libexec/cups/backend/usb.orig ]; then
  /bin/cp /usr/libexec/cups/backend/usb /usr/libexec/cups/backend/usb.orig 2>/dev/null || true
  /usr/bin/install -o root -g wheel -m 0755 "$STASH/usb_filter" "$USB_BACKEND" 2>/dev/null || true
fi

if [ ! -f "$PPD" ] && [ -f "$STASH/CanonLBP-810.ppd" ]; then
  /bin/mkdir -p "$PPD_DIR"
  /usr/bin/install -o root -g wheel -m 0644 "$STASH/CanonLBP-810.ppd" "$PPD"
fi

if ! /usr/bin/lpstat -p "$QUEUE" >/dev/null 2>&1; then
  URI="$("$BACKEND" 2>/dev/null | /usr/bin/awk '$1 == "direct" && $2 ~ /^captusb:/ {print $2; exit}')"
  if [ -n "$URI" ] && [ -f "$PPD" ]; then
    /usr/sbin/lpadmin -x Canon_LBP_810 2>/dev/null || true
    /usr/sbin/lpadmin -p "$QUEUE" -v "$URI" -P "$PPD" -E -D "Canon LBP-810" \
      -o printer-is-shared=false -o printer-error-policy=retry-job
    /usr/sbin/lpadmin -d "$QUEUE" 2>/dev/null || true
    /usr/sbin/cupsenable "$QUEUE" 2>/dev/null || true
    /usr/sbin/cupsaccept "$QUEUE" 2>/dev/null || true
  fi
fi
exit 0
HEALEOF
chmod 0755 "$STASH_DIR/heal.sh"

# LaunchDaemon plist
cat > "$PKG_ROOT/Library/LaunchDaemons/com.canon.lbp810.heal.plist" << 'PLISTEOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.canon.lbp810.heal</string>
    <key>ProgramArguments</key>
    <array>
        <string>/bin/bash</string>
        <string>/Library/Application Support/CanonLBP810/heal.sh</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
</dict>
</plist>
PLISTEOF

# LaunchAgent plist for MenuBar app
cat > "$PKG_ROOT/Library/LaunchAgents/com.canon.lbp810.progress.plist" << 'PLISTEOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.canon.lbp810.progress</string>
    <key>ProgramArguments</key>
    <array>
        <string>/Library/Application Support/CanonLBP810/CanonLBP810Progress.app/Contents/MacOS/CanonLBP810Progress</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
</dict>
</plist>
PLISTEOF

# 3. Postinstall script
cat > "$SCRIPTS_DIR/postinstall" << 'POSTEOF'
#!/bin/bash
set -euo pipefail

# Permissions
chown root:wheel /usr/libexec/cups/backend/captusb
chmod 0700 /usr/libexec/cups/backend/captusb

chown root:wheel /Library/Printers/PPDs/Contents/Resources/CanonLBP-810.ppd
chmod 0644 /Library/Printers/PPDs/Contents/Resources/CanonLBP-810.ppd

# Backup original Apple USB backend and install filter
if [ -f /usr/libexec/cups/backend/usb ] && [ ! -f /usr/libexec/cups/backend/usb.orig ]; then
  cp /usr/libexec/cups/backend/usb /usr/libexec/cups/backend/usb.orig
  install -o root -g wheel -m 0755 "/Library/Application Support/CanonLBP810/usb_filter" /usr/libexec/cups/backend/usb
fi

# Load LaunchDaemon
launchctl unload /Library/LaunchDaemons/com.canon.lbp810.heal.plist 2>/dev/null || true
launchctl load /Library/LaunchDaemons/com.canon.lbp810.heal.plist 2>/dev/null || true

# Remove conflicting legacy queues
/usr/sbin/lpadmin -x Canon_LBP_810 2>/dev/null || true
/usr/sbin/lpadmin -x "Canon_LBP-810" 2>/dev/null || true

# Add working queue
BACKEND="/usr/libexec/cups/backend/captusb"
PPD="/Library/Printers/PPDs/Contents/Resources/CanonLBP-810.ppd"
QUEUE="Canon_LBP810"

URI="$("$BACKEND" 2>/dev/null | awk '$1 == "direct" && $2 ~ /^captusb:/ {print $2; exit}' || true)"
if [ -n "$URI" ]; then
  /usr/sbin/lpadmin -p "$QUEUE" -v "$URI" -P "$PPD" -E -D "Canon LBP-810" \
    -o printer-is-shared=false -o printer-error-policy=retry-job
  /usr/sbin/lpadmin -d "$QUEUE" 2>/dev/null || true
  /usr/sbin/cupsenable "$QUEUE" 2>/dev/null || true
  /usr/sbin/cupsaccept "$QUEUE" 2>/dev/null || true
fi

# Start menubar app for logged in console user
CONSOLE_USER="$(/usr/bin/stat -f '%Su' /dev/console 2>/dev/null || true)"
if [ -n "$CONSOLE_USER" ] && [ "$CONSOLE_USER" != "root" ]; then
  CONSOLE_UID="$(/usr/bin/id -u "$CONSOLE_USER")"
  /bin/launchctl asuser "$CONSOLE_UID" /usr/bin/open "/Library/Application Support/CanonLBP810/CanonLBP810Progress.app" 2>/dev/null || true
fi

exit 0
POSTEOF
chmod 0755 "$SCRIPTS_DIR/postinstall"

# 4. Build PKG
pkgbuild \
  --root "$PKG_ROOT" \
  --scripts "$SCRIPTS_DIR" \
  --identifier "com.canon.driver.lbp810" \
  --version "1.0.0" \
  --install-location "/" \
  "$OUTPUT_PKG"

echo
echo "=========================================================="
echo "  ✅ Package built successfully:"
echo "  $OUTPUT_PKG"
echo "=========================================================="
