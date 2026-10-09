#!/bin/sh
# builds a temporary tree with the real display headers and stubbed dependencies
set -e
T=$1; SRC=$2
rm -rf $T; mkdir -p $T/src/plugins/Display $T/src/utils $T/src/config
cp $SRC/plugins/Display/Display_Mono.h $SRC/plugins/Display/Display_Mono_128X64.h $SRC/plugins/Display/Display_data.h $SRC/plugins/Display/Display_icons.h $T/src/plugins/Display/
cp $SRC/utils/timemonitor.h $T/src/utils/
echo '#pragma once' > $T/src/plugins/Display/Display.h
printf '#pragma once\n#include <Arduino.h>\n#include <TimeLib.h>\nnamespace ah { inline String getDateTimeStrShort_i18n(uint32_t){return String("");} }\n' > $T/src/utils/helper.h
printf '#pragma once\n#define DBGPRINTLN(x)\n#define F(x) x\n' > $T/src/utils/dbg.h
grep -n "DISP_TYPE" $SRC/defines.h | head -0
awk '/^enum \{/{p=1} p{print} p&&/^\};/{exit}' $SRC/defines.h > $T/src/disp_enum.h
printf '#pragma once\n#include <cstdint>\n#include "disp_enum.h"\n' > $T/src/defines.h
awk '/^typedef struct \{/{buf=""; p=1} p{buf=buf $0 "\n"} p&&/\} display_t;/{print buf; exit} p&&/^\} [a-zA-Z_]+;/{p=0}' $SRC/config/settings.h > $T/src/config/disp_t.h
printf '#pragma once\n#include <cstdint>\n#include "disp_t.h"\n' > $T/src/config/settings.h
