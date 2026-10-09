#!/bin/sh
# usage: shot.sh <url> <out.png> <width> <height>
/opt/google/chrome/chrome --headless=new --no-sandbox --disable-gpu --hide-scrollbars \
  --force-device-scale-factor=2 --window-size=$3,$4 --virtual-time-budget=4000 \
  --screenshot=$2 "$1" 2>/dev/null
