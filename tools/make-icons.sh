#!/bin/sh
# Renders the extension's PNG icons from icons/icon.svg (Chromium wants PNGs).
set -eu
cd "$(dirname "$0")/../extension/icons"
for size in 16 32 48 128; do
  rsvg-convert -w "$size" -h "$size" icon.svg -o "icon-$size.png"
done
