// PC preview for the modern 128x64 display layout.
// Compiles the real Display_Mono_128X64.h against the real u8g2 library and
// writes the frame buffer of several scenarios as PBM images.
#include <U8g2lib.h>
#include <cstdio>
#include <cmath>
#include <ctime>

uint32_t gMillis = 0;
uint32_t millis() { return gMillis; }

// replaces the Arduino I2C display classes by an in-memory display
class TestDisp : public U8G2 {
    public:
        TestDisp(const u8g2_cb_t *rot, uint8_t, uint8_t, uint8_t) : U8G2() {
            u8g2_Setup_sh1106_128x64_noname_f(&u8g2, rot, u8x8_byte_empty, u8x8_dummy_cb);
        }
};
#define U8G2_SSD1306_128X64_NONAME_F_HW_I2C TestDisp
#define U8G2_SH1106_128X64_NONAME_F_HW_I2C TestDisp
#define U8G2_SSD1309_128X64_NONAME0_F_HW_I2C TestDisp

#include "plugins/Display/Display_Mono_128X64.h"

static U8G2 *gDisp = nullptr;
class Probe : public DisplayMono128X64 {
    public:
        U8G2 *u8() { return mDisplay; }
};

static void savePbm(U8G2 *d, const char *name) {
    uint8_t *buf = d->getBufferPtr();
    int w = 128, h = 64;
    FILE *f = fopen(name, "w");
    fprintf(f, "P1\n%d %d\n", w, h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t b = buf[(y / 8) * w + x];
            fputc((b >> (y & 7)) & 1 ? '1' : '0', f);
        }
        fputc('\n', f);
    }
    fclose(f);
}

// fake day curve: bell shape with a few clouds
static void makeCurve(float *v, int width, float peak) {
    for (int i = 0; i < width; i++) {
        float t = (float)i / (width - 1);
        float s = sinf(t * 3.14159f);
        float val = peak * powf(s, 1.6f);
        if (i > 38 && i < 46) val *= 0.55f;
        if (i > 70 && i < 74) val *= 0.7f;
        v[i] = val;
    }
}

int main() {
    display_t cfg{};
    cfg.type = DISP_TYPE_T2_SH1106_128X64;
    cfg.graph_ratio = 30;
    cfg.screenSaver = 0;

    DisplayData dd;
    dd.version = "0.8.156";
    dd.ipAddress = IPAddress(192, 168, 1, 77);
    dd.WifiSymbol = true; dd.WifiRSSI = -58;
    dd.RadioSymbol = true; dd.RadioRSSI = -65;
    dd.MQTTSymbol = true;

    // 2026-10-09 local, sunrise 07:05, sunset 18:20
    uint32_t day0 = 1791504000;  // 2026-10-09 00:00 (treated as local)
    dd.pGraphStartTime = day0 + 7 * 3600 + 5 * 60;
    dd.pGraphEndTime = day0 + 18 * 3600 + 20 * 60;

    Probe p;
    p.config(&cfg);
    p.init(&dd);
    savePbm(p.u8(), "00_splash.pbm");

    float curve[128];
    makeCurve(curve, 128 - 0, 1680.0f);
    auto at = [&](int hh, int mm) { return day0 + hh * 3600 + mm * 60; };
    auto posOf = [&](uint32_t ts) { return (uint8_t)((ts - dd.pGraphStartTime) * 127 / (dd.pGraphEndTime - dd.pGraphStartTime)); };

    // 1) midday
    dd.utcTs = at(13, 24); dd.nrProducing = 2; dd.nrSleeping = 0;
    dd.totalPower = 1243; dd.totalYieldDay = 6420; dd.totalYieldTotal = 2843.6;
    p.previewSetCurve(curve, posOf(dd.utcTs), dd.pGraphStartTime, dd.pGraphEndTime);
    gMillis = 1000; p.disp(); savePbm(p.u8(), "01_mittag.pbm");

    // 2) morning, small power
    dd.utcTs = at(9, 2); dd.totalPower = 318; dd.totalYieldDay = 412; dd.totalYieldTotal = 2837.3;
    p.previewSetCurve(curve, posOf(dd.utcTs), dd.pGraphStartTime, dd.pGraphEndTime);
    p.disp(); savePbm(p.u8(), "02_morgen.pbm");

    // 3) one inverter sleeping, weak wifi, no mqtt
    dd.utcTs = at(16, 41); dd.totalPower = 587; dd.nrProducing = 1; dd.nrSleeping = 1;
    dd.totalYieldDay = 11870; dd.WifiRSSI = -76; dd.MQTTSymbol = false;
    p.previewSetCurve(curve, posOf(dd.utcTs), dd.pGraphStartTime, dd.pGraphEndTime);
    p.disp(); savePbm(p.u8(), "03_teilweise.pbm");

    // 4) evening, all sleeping
    dd.utcTs = at(19, 12); dd.nrProducing = 0; dd.nrSleeping = 2; dd.totalPower = 0;
    dd.totalYieldDay = 12340; dd.WifiRSSI = -58; dd.MQTTSymbol = true; dd.RadioSymbol = true;
    p.previewSetCurve(curve, 127, dd.pGraphStartTime, dd.pGraphEndTime);
    p.disp(); savePbm(p.u8(), "04_nacht.pbm");

    // 5) detail page
    dd.utcTs = at(13, 24); dd.nrProducing = 2; dd.nrSleeping = 0; dd.totalPower = 1243; dd.totalYieldDay = 6420;
    p.previewSetCurve(curve, posOf(dd.utcTs), dd.pGraphStartTime, dd.pGraphEndTime);
    p.previewSetSwitchState(true);
    p.disp(); savePbm(p.u8(), "05_kurve.pbm");
    p.previewSetSwitchState(false);

    // 6) big value, radio problem
    dd.utcTs = at(12, 5); dd.totalPower = 12480; dd.RadioSymbol = false;
    p.previewSetCurve(curve, posOf(dd.utcTs), dd.pGraphStartTime, dd.pGraphEndTime);
    p.disp(); savePbm(p.u8(), "06_gross_funkfehler.pbm");

    // 7) no inverter configured, no radio, freshly booted (empty curve)
    dd.nrProducing = 0; dd.nrSleeping = 0; dd.totalPower = 0; dd.totalYieldDay = 0; dd.totalYieldTotal = 0;
    dd.RadioSymbol = false; dd.MQTTSymbol = false; dd.APSymbol = true; dd.utcTs = at(20, 41);
    { float z[128] = {0}; p.previewSetCurve(z, 0, dd.pGraphStartTime, dd.pGraphEndTime); }
    p.disp(); savePbm(p.u8(), "07_kein_wechselrichter.pbm");

    // 8) producing, but WiFi lost and radio module failing
    dd.nrProducing = 1; dd.nrSleeping = 0; dd.totalPower = 734; dd.totalYieldDay = 3120; dd.totalYieldTotal = 2841.0;
    dd.WifiSymbol = false; dd.RadioSymbol = true; dd.RadioRSSI = -75; dd.APSymbol = true; dd.MQTTSymbol = false; dd.utcTs = at(11, 17);
    p.previewSetCurve(curve, posOf(dd.utcTs), dd.pGraphStartTime, dd.pGraphEndTime);
    p.disp(); savePbm(p.u8(), "08_kein_wlan.pbm");
    return 0;
}
