#!/bin/sh
# usage: ./build.sh <u8g2-checkout> <outdir>
set -e
U8=$1; OUT=$2; HERE=$(cd $(dirname $0); pwd)
T=$OUT/tree
mkdir -p $OUT
$HERE/setup_tree.sh $T $HERE/../../src
if [ ! -f $OUT/libu8g2.a ]; then
  (cd $OUT && gcc -O2 -c -I$U8/csrc $U8/csrc/*.c && ar rcs libu8g2.a *.o && rm -f *.o)
fi
g++ -std=gnu++17 -O1 -Wall -Wno-unused-variable -DDISPLAY_PREVIEW_HOST -DU8X8_NO_HW_SPI -DU8X8_NO_HW_I2C \
  -I$HERE/stubs -I$T/src -I$U8/cppsrc -I$U8/csrc \
  $HERE/preview.cpp $U8/cppsrc/U8g2lib.cpp $U8/cppsrc/U8x8lib.cpp $OUT/libu8g2.a -o $OUT/preview
(cd $OUT && ./preview)
