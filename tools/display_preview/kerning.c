// computes x positions for letters so that the visible gap between all letters is equal
// usage: kerning <gap> <text...>   (font: u8g2_font_luIS10_tr)
#include "u8g2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static u8g2_t u;
static void extent(char ch, int *l, int *r) {   // leftmost / rightmost pixel column relative to draw x=20
    u8g2_ClearBuffer(&u); char s[2] = {ch, 0};
    u8g2_DrawStr(&u, 20, 30, s);
    uint8_t *b = u8g2_GetBufferPtr(&u); *l = 999; *r = -999;
    for (int x = 0; x < 128; x++) for (int y = 0; y < 64; y++)
        if ((b[(y/8)*128+x] >> (y&7)) & 1) { if (x - 20 < *l) *l = x - 20; if (x - 20 > *r) *r = x - 20; }
}
int main(int argc, char **argv) {
    u8g2_Setup_sh1106_128x64_noname_f(&u, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);
    u8g2_InitDisplay(&u); u8g2_SetFont(&u, u8g2_font_luIS10_tr); u8g2_SetFontMode(&u, 1);
    int gap = atoi(argv[1]);
    for (int a = 2; a < argc; a++) {
        const char *t = argv[a]; int x = 0, prevR = 0, first = 1;
        printf("// \"%s\"\n{", t);
        for (int i = 0; t[i]; i++) {
            int l, r;
            if (' ' == t[i]) { prevR += 3 * gap; printf(" %d,", x); continue; }
            extent(t[i], &l, &r);
            x = first ? -l : prevR + gap + 1 - l; first = 0;
            prevR = x + r;
            printf(" %d,", x);
        }
        printf(" } // width %d\n", prevR + 1);
    }
}
