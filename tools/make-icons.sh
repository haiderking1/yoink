#!/bin/sh
# Renders the extension's PNG icons from the app icon (Chromium wants PNGs).
set -eu
cd "$(dirname "$0")/.."
for size in 16 32 48 128; do
  rsvg-convert -w "$size" -h "$size" data/icons/dev.soka.dy.svg -o "extension/icons/icon-$size.png"
done
