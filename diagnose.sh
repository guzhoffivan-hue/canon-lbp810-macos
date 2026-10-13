#!/bin/bash
#
# Diagnostic script for Canon LBP-810 on macOS
# Usage: ./diagnose.sh
#
set -euo pipefail

echo "=========================================================="
echo "    Canon LBP-810 Diagnostic Utility"
echo "=========================================================="

echo "--- 1. System Info ---"
sw_vers
uname -m

echo
echo "--- 2. USB Hardware Detection ---"
USB_INFO=$(ioreg -p IOUSB -l -w0 -r -c IOUSBHostDevice | grep -A 15 -B 2 "Canon CAPT USB Printer" || true)
if [ -n "$USB_INFO" ]; then
  echo "✅ Canon LBP-810 hardware found on USB:"
  echo "$USB_INFO" | grep -E 'idVendor|idProduct|kUSBSerialNumberString|USBSpeed'
else
  echo "❌ Canon LBP-810 is NOT detected on USB bus."
  echo "   Check physical USB cable, power switch, and adapter."
fi

echo
echo "--- 3. CUPS Backend Status ---"
if [ -x /usr/libexec/cups/backend/captusb ]; then
  echo "✅ Backend binary is installed: /usr/libexec/cups/backend/captusb"
  ls -la /usr/libexec/cups/backend/captusb
  echo "Running discovery:"
  /usr/libexec/cups/backend/captusb || true
else
  echo "⚠️ Backend /usr/libexec/cups/backend/captusb is not yet installed."
  if [ -x "./prebuilt/arm64/captusb" ]; then
    echo "Running local prebuilt captusb:"
    ./prebuilt/arm64/captusb || true
  fi
fi

echo
echo "--- 4. CUPS Queues and Status ---"
/usr/bin/lpstat -r || true
echo "Queues:"
/usr/bin/lpstat -p Canon_LBP810 2>/dev/null || echo "Queue Canon_LBP810 not found."
/usr/bin/lpstat -v Canon_LBP810 2>/dev/null || true

echo
echo "--- 5. Recent CUPS Error Log Entries ---"
if [ -f /private/var/log/cups/error_log ]; then
  grep -iE 'capt|lbp|canon' /private/var/log/cups/error_log | tail -n 15 || echo "No relevant error log entries."
else
  echo "No CUPS error_log found."
fi

echo "=========================================================="
