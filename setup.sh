#!/bin/bash
#
# Canon LBP-810 Driver Setup for macOS (Apple Silicon / ARM64)
# Run without sudo: ./setup.sh
#
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$(id -u)" -eq 0 ]; then
  echo "Please run WITHOUT sudo: ./setup.sh" >&2
  echo "(it will ask for your administrator password when needed)" >&2
  exit 1
fi

echo "=========================================================="
echo "    Canon LBP-810 Driver for macOS (Apple Silicon)"
echo "=========================================================="

# Remove quarantine attributes if downloaded as archive
xattr -dr com.apple.quarantine "$DIR" 2>/dev/null || true

echo "==> Running installer with administrator privileges..."
sudo bash "$DIR/install.sh"

echo
echo "To test printing, run:"
echo "  lpr -P Canon_LBP810 /path/to/document.pdf"
echo "or print directly from Preview, Word, or any macOS app."
