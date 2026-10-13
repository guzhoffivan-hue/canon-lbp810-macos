#!/bin/bash
#
# Canon LBP-810 Driver Uninstaller for macOS
# Usage: sudo ./uninstall.sh
#
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
  echo "Error: please run as administrator: sudo ./uninstall.sh" >&2
  exit 1
fi

QUEUE_NAME="Canon_LBP810"
DAEMON_LABEL="com.canon.lbp810.heal"
DAEMON_PLIST="/Library/LaunchDaemons/$DAEMON_LABEL.plist"

echo "=========================================================="
echo "    Uninstalling Canon LBP-810 Driver"
echo "=========================================================="

# 1. Remove queue
if /usr/sbin/lpstat -p "$QUEUE_NAME" >/dev/null 2>&1; then
  echo "==> Removing CUPS queue '$QUEUE_NAME'..."
  /usr/sbin/lpadmin -x "$QUEUE_NAME"
fi

# 2. Stop and remove LaunchDaemon
if [ -f "$DAEMON_PLIST" ]; then
  echo "==> Removing self-healing LaunchDaemon..."
  launchctl unload "$DAEMON_PLIST" 2>/dev/null || true
  rm -f "$DAEMON_PLIST"
fi

# 3. Remove backend and PPD
echo "==> Removing driver files..."
rm -f /usr/libexec/cups/backend/captusb
rm -f /Library/Printers/PPDs/Contents/Resources/CanonLBP-810.ppd
rm -f /usr/share/cups/model/LBP810CAPTPPD.ppd 2>/dev/null || true

# 4. Remove stash
rm -rf "/Library/Application Support/CanonLBP810"

echo "=========================================================="
echo "  ✅ Driver and print queue successfully uninstalled."
echo "=========================================================="
