//-----------------------------------------------------------------------------
// 2023 Ahoy, https://ahoydtu.de
// Creative Commons - https://creativecommons.org/licenses/by-nc-sa/4.0/deed
//-----------------------------------------------------------------------------
// Modern layout for 128x64 displays (SSD1306 / SH1106 / SSD1309):
//  - status bar (time, radio bars, WiFi, MQTT) like a phone status bar
//  - large power value on top of a dithered day curve (sunrise .. sunset)
//  - day / total yield at the bottom
//  - optional detail page with the day curve (display setting "graph ratio")
//-----------------------------------------------------------------------------

#pragma once
#include "Display.h"
#include "Display_Mono.h"
#include "Display_icons.h"

class DisplayMono128X64 : public DisplayMono {
    public:
        DisplayMono128X64() : DisplayMono() {
            mExtra = 0;
        }

        void config(display_t *cfg) override {
            mCfg = cfg;
        }

        // called every second: refresh at once when the status bar switches between IP and time
        bool loop(bool motion) override {
            bool refresh = DisplayMono::loop(motion);
            bool phase = ipPhase();
            if (phase != mLastIpPhase) {
                mLastIpPhase = phase;
                refresh = true;
            }
            return refresh;
        }

        void init(DisplayData *displayData) override {
            u8g2_cb_t *rot = (u8g2_cb_t *)(( mCfg->rot != 0x00) ? U8G2_R2 : U8G2_R0);
            switch (mCfg->type) {
                case DISP_TYPE_T1_SSD1306_128X64:
                    monoInit(new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(rot, 0xff, mCfg->disp_clk, mCfg->disp_data), displayData);
                    break;
                case DISP_TYPE_T2_SH1106_128X64:
                    monoInit(new U8G2_SH1106_128X64_NONAME_F_HW_I2C(rot, 0xff, mCfg->disp_clk, mCfg->disp_data), displayData);
                    break;
                case DISP_TYPE_T6_SSD1309_128X64:
                default:
                    monoInit(new U8G2_SSD1309_128X64_NONAME0_F_HW_I2C(rot, 0xff, mCfg->disp_clk, mCfg->disp_data), displayData);
                    break;
            }
            mDisplay->setFontMode(1);       // transparent font background
            mDisplay->setBitmapMode(1);     // transparent bitmap background

            widthShrink = (mCfg->screenSaver == 1) ? pixelShiftRange : 0;  // shrink width for pixelshift screensaver

            // the day curve is always recorded: it is the background of the main page
            initPowerGraph(mDispWidth - widthShrink, CURVE_H_DETAIL);

            drawSplash();
        }

        void disp(void) override {
            mDisplay->clearBuffer();

            // calculate current pixelshift for pixelshift screensaver
            calcPixelShift(pixelShiftRange);

            // add new power data to the day curve
            if (mDisplayData->nrProducing > 0)
                addPowerGraphEntry(mDisplayData->totalPower);

            if ((mCfg->graph_ratio > 0) && (mDispSwitchState == DispSwitchState::GRAPH))
                drawCurvePage();
            else
                drawMainPage();

            drawStatusBar();
            mDisplay->sendBuffer();

            mExtra++;
        }

    private:
        bool mLastIpPhase = false;

        // true while the status bar should show the IP address (equal time slots: N s IP, N s time)
        bool ipPhase(void) {
            uint8_t t = mCfg->ipTime;
            if (0 == t)
                return false;
            return (1 == ((millis() / 1000UL / t) % 2));
        }

        static constexpr uint8_t pixelShiftRange = 11;  // number of pixels to shift from left to right (centered -> must be odd!)
        uint8_t widthShrink = 0;

        // layout (y = baselines)
        static constexpr uint8_t Y_STATUS        = 8;   // status bar text baseline
        static constexpr uint8_t Y_VALUE         = 35;  // big power value baseline
        static constexpr uint8_t Y_CURVE_MAIN    = 50;  // baseline of the background curve
        static constexpr uint8_t CURVE_H_MAIN    = 17;  // band below the value
        static constexpr uint8_t CURVE_H_NIGHT   = 11;
        static constexpr uint8_t Y_BOTTOM        = 63;  // yield row baseline
        static constexpr uint8_t Y_CURVE_DETAIL  = 55;
        static constexpr uint8_t CURVE_H_DETAIL  = 40;

        inline int16_t x0(void) { return (widthShrink / 2) + mPixelshift; }   // left content edge
        inline int16_t x1(void) { return mDispWidth - 1 - (widthShrink / 2) + mPixelshift; } // right content edge
        inline int16_t xc(void) { return mDispWidth / 2 + mPixelshift; }      // center

        //---------------------------------------------------------------------
#ifndef SPLASH_SIGN_GAP
#define SPLASH_SIGN_GAP 2
#endif
#ifndef SPLASH_SIGN_FONT
#define SPLASH_SIGN_FONT u8g2_font_luIS10_tr
#endif
        void drawSplash(void) {
            mDisplay->clearBuffer();

            // title
            mDisplay->setFont(u8g2_font_helvB12_tr);
            drawCentered("AHOY DTU", mDispWidth / 2, 16);
            mDisplay->drawHLine(mDispWidth / 2 - 32, 20, 64);

            // version, small, below the title
            mDisplay->setFont(u8g2_font_4x6_tr);
            snprintf(mFmtText, DISP_FMT_TEXT_LEN, "v%s", (nullptr != mDisplayData->version) ? mDisplayData->version : "");
            drawCentered(mFmtText, mDispWidth / 2, 28);

            // signature, "scribbled": every letter jumps a little up or down
            mDisplay->setFont(SPLASH_SIGN_FONT);
            // letter positions computed with tools/display_preview/kerning.c -> equal gaps between all letters
#if (SPLASH_SIGN_GAP == 1)
            static const int8_t posA[] = {-2, 6, 13, 22, 29, 35, 40, 49, 49, 59, 68};      // width 77
            static const int8_t posB[] = {-1, 12, 19, 26, 31, 39, 46, 53, 58, 64, 72, 78}; // width 86
            const int16_t wB = 86;
#else
            static const int8_t posA[] = {-2, 7, 15, 25, 33, 40, 46, 56, 56, 70, 80};      // width 89
            static const int8_t posB[] = {-1, 13, 21, 29, 35, 44, 52, 60, 66, 73, 82, 89}; // width 97
            const int16_t wB = 97;
#endif
            drawScribble("redesign by", posA, 4, 44, 0);
            drawScribble("mariofritzer", posB, mDispWidth - 3 - wB, 59, 3);
            mDisplay->sendBuffer();
        }

        // draws text letter by letter at fixed x positions with a small vertical jitter -> handwritten look
        void drawScribble(const char *txt, const int8_t *pos, int16_t x, int16_t y, uint8_t seed) {
            static const int8_t jitter[] = {0, -1, 0, 1, 1, 0, -1, 0, 1, -1, 0, 1};
            char c[2] = {0, 0};
            for (uint8_t i = 0; txt[i]; i++) {
                if (' ' == txt[i]) continue;
                c[0] = txt[i];
                mDisplay->drawStr(x + pos[i], y + jitter[(i + seed) % sizeof(jitter)], c);
            }
        }

        //---------------------------------------------------------------------
        // status bar: time (or IP) on the left, radio / WiFi / MQTT on the right
        void drawStatusBar(void) {
            mDisplay->setFont(u8g2_font_helvB08_tr);
            bool showIp = (0 == mDisplayData->utcTs) || ipPhase();
            if (showIp && (mDisplayData->ipAddress != IPAddress(0, 0, 0, 0)))
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%s", mDisplayData->ipAddress.toString().c_str());
            else if (0 != mDisplayData->utcTs)
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%02d:%02d", hour(mDisplayData->utcTs), minute(mDisplayData->utcTs));
            else
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "--:--");
            drawKnockoutStr(x0() + 1, Y_STATUS, mFmtText);

            // icons from right to left. rule: normal = white icon, problem = inverted box
            int16_t x = x1() + 1;

            // WiFi: 3 bold arcs + dot, unlit arcs dimmed (dithered)
            x -= ICON_WIFI_DOT_W;
            if (mDisplayData->WifiSymbol) {
                int8_t wl = 1;
                if (mDisplayData->WifiRSSI > -80) wl = 2;
                if (mDisplayData->WifiRSSI > -70) wl = 3;
                if (mDisplayData->WifiRSSI > -60) wl = 4;
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_dot,   true);
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_in,    wl >= 2);
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_mid,   wl >= 3);
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_outer, wl >= 4);
            } else {
                // no WiFi connection: full icon, struck through
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_dot,   true);
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_in,    true);
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_mid,   true);
                drawLevelIcon(x, 0, ICON_WIFI_DOT_W, ICON_WIFI_DOT_H, icon_wifi_outer, true);
                strikeThrough(x, ICON_WIFI_DOT_W);
            }

            // hotspot (access point) active: "AP" in a rounded frame
            if (mDisplayData->APSymbol) {
                mDisplay->setFont(u8g2_font_5x7_tr);
                x -= 3 + 14;
                mDisplay->drawRFrame(x, 0, 14, 10, 2);
                mDisplay->drawStr(x + 2, 8, "AP");
            }

            // MQTT: "M" in a rounded frame, only when connected
            if (mDisplayData->MQTTSymbol) {
                mDisplay->setFont(u8g2_font_5x7_tr);
                x -= 4 + 9;
                mDisplay->drawRFrame(x, 0, 9, 10, 2);
                mDisplay->drawStr(x + 2, 8, "M");
            }

            // radio to the inverters: antenna + 4 bars like a mobile phone
            x -= 4 + 4 * 3 - 1;
            int16_t xBars = x;
            if (mDisplayData->RadioSymbol) {
                for (uint8_t i = 0; i < 4; i++) {
                    uint8_t h = 3 + i * 2;   // 3, 5, 7, 9
                    int16_t bx = x + i * 3;
                    bool lit = (mDisplayData->RadioRSSI > (-60 - (3 - i) * 10));
                    if (lit)
                        mDisplay->drawBox(bx, 10 - h, 2, h);
                    else
                        mDisplay->drawBox(bx, 9, 2, 1);   // unlit bar: short line on the baseline
                }
                x -= ICON_ANTENNA_W + 1;
                mDisplay->drawXBMP(x, 0, ICON_ANTENNA_W, ICON_ANTENNA_H, icon_antenna);
            } else {
                // radio module not found / not working: antenna + empty bars, struck through
                for (uint8_t i = 0; i < 4; i++)
                    mDisplay->drawBox(x + i * 3, 9, 2, 1);
                x -= ICON_ANTENNA_W + 1;
                mDisplay->drawXBMP(x, 0, ICON_ANTENNA_W, ICON_ANTENNA_H, icon_antenna);
                strikeThrough(x - 1, ICON_ANTENNA_W + 2);
            }

            // some inverters sleeping: framed badge "producing/total"
            if ((mDisplayData->nrSleeping > 0) && (mDisplayData->nrProducing > 0)) {
                mDisplay->setFont(u8g2_font_5x7_tr);
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%d/%d", mDisplayData->nrProducing,
                         mDisplayData->nrProducing + mDisplayData->nrSleeping);
                uint8_t w = mDisplay->getStrWidth(mFmtText);
                int16_t bx = x - w - 9;
                mDisplay->drawRFrame(bx, 0, w + 5, 10, 2);
                mDisplay->drawStr(bx + 3, 8, mFmtText);
            }
        }

        // diagonal strike through an icon (bottom left -> top right) with a dark gap
        // on both sides, so the line stays visible on top of the icon
        void strikeThrough(int16_t x, uint8_t w) {
            int16_t x2 = x + w - 1;
            mDisplay->setDrawColor(0);
            mDisplay->drawLine(x, 8, x2 - 1, -1);
            mDisplay->drawLine(x + 1, 10, x2, 1);
            mDisplay->setDrawColor(1);
            mDisplay->drawLine(x, 9, x2, 0);
        }

        //---------------------------------------------------------------------
        // main page: big power value on top of the day curve, yields at the bottom
        void drawMainPage(void) {
            bool producing = (mDisplayData->nrProducing > 0);
            plotDayCurve(x0(), Y_CURVE_MAIN, producing ? CURVE_H_MAIN : CURVE_H_NIGHT, 0, producing);

            if (0 == mDisplayData->nrSleeping + mDisplayData->nrProducing) {
                mDisplay->setFont(u8g2_font_helvB10_tr);
                drawHaloStr(xc() - mDisplay->getStrWidth("Willkommen") / 2, 27, "Willkommen", 2);
                mDisplay->setFont(u8g2_font_helvB08_tr);
                drawKnockoutCentered("Kein Wechselrichter", xc(), 39);
            }
            else if (mDisplayData->nrProducing > 0)
                drawPowerValue();
            else
                drawOffline();

            drawYieldRow();
        }

        void drawPowerValue(void) {
            const char *unit;
            float p = mDisplayData->totalPower;
            if (p >= 10000.0f) {
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%.1f", p / 1000.0f);
                unit = "kW";
            } else if (p >= 1000.0f) {
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%.2f", p / 1000.0f);
                unit = "kW";
            } else {
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%.0f", p);
                unit = "W";
            }
            decimalComma(mFmtText);

            mDisplay->setFont(u8g2_font_logisoso22_tn);
            uint8_t wVal = mDisplay->getStrWidth(mFmtText);
            mDisplay->setFont(u8g2_font_helvB10_tr);
            uint8_t wUnit = mDisplay->getStrWidth(unit);
            int16_t x = xc() - (wVal + 2 + wUnit) / 2;

            mDisplay->setFont(u8g2_font_logisoso22_tn);
            drawHaloStr(x, Y_VALUE, mFmtText, 2);
            mDisplay->setFont(u8g2_font_helvB10_tr);
            drawHaloStr(x + wVal + 2, Y_VALUE, unit, 2);

        }

        void drawOffline(void) {
            bool night = (0 != mDisplayData->utcTs) && (0 != mDisplayData->pGraphStartTime) &&
                         ((mDisplayData->utcTs < mDisplayData->pGraphStartTime) ||
                          (mDisplayData->utcTs > mDisplayData->pGraphEndTime));

            mDisplay->setFont(u8g2_font_helvB10_tr);
            const char *title = night ? "Nachtruhe" : "Offline";
            uint8_t w = mDisplay->getStrWidth(title) + ICON_MOON_W + 5;
            int16_t x = xc() - w / 2;
            drawHaloXBM(x, 15, ICON_MOON_W, ICON_MOON_H, icon_moon);
            drawHaloStr(x + ICON_MOON_W + 5, 27, title, 2);

            mDisplay->setFont(u8g2_font_helvB08_tr);
            if (night && (0 != mDisplayData->pGraphStartTime) && (mDisplayData->utcTs < mDisplayData->pGraphStartTime))
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "Sonnenaufgang %02d:%02d", hour(mDisplayData->pGraphStartTime), minute(mDisplayData->pGraphStartTime));
            else if (night)
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "Wechselrichter schlafen");
            else
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "Keine Verbindung");
            drawKnockoutCentered(mFmtText, xc(), 37);
        }

        void drawYieldRow(void) {
            // separator: thin line, only where the curve is not
            mDisplay->setDrawColor(0);
            mDisplay->drawBox(0, Y_CURVE_MAIN + 1, mDispWidth, mDisplay->getDisplayHeight() - Y_CURVE_MAIN - 1);
            mDisplay->setDrawColor(1);

            // the two values must never touch: try normal precision, then without decimals,
            // then a smaller font (pixel shift screensaver makes the row 11 px narrower)
            char day[16], tot[16];
            const int16_t avail = x1() - x0() + 1;
            uint8_t level = 0;
            for (; level < 3; level++) {
                formatYieldDay(day, sizeof(day), mDisplayData->totalYieldDay, level > 0);
                formatYieldTotal(tot, sizeof(tot), mDisplayData->totalYieldTotal, level > 0);
                mDisplay->setFont((level < 2) ? u8g2_font_helvB08_tr : u8g2_font_5x7_tr);
                int16_t need = 1 + ICON_SUN_W + 2 + mDisplay->getStrWidth(day) + 4
                             + ICON_SIGMA_W + 2 + mDisplay->getStrWidth(tot) + 1;
                if (need <= avail)
                    break;
            }

            // day yield (left)
            mDisplay->drawXBMP(x0() + 1, Y_BOTTOM - 8, ICON_SUN_W, ICON_SUN_H, icon_sun);
            mDisplay->drawStr(x0() + ICON_SUN_W + 3, Y_BOTTOM, day);

            // total yield (right aligned)
            int16_t xr = x1() - mDisplay->getStrWidth(tot);
            mDisplay->drawStr(xr, Y_BOTTOM, tot);
            mDisplay->drawXBMP(xr - ICON_SIGMA_W - 2, Y_BOTTOM - 8, ICON_SIGMA_W, ICON_SIGMA_H, icon_sigma);
        }

        // day yield in Wh -> "412 Wh", "9,99 kWh", "12,3 kWh" (short: no decimals)
        static void formatYieldDay(char *buf, size_t len, float wh, bool shortFmt) {
            if (wh >= 1000.0f) {
                float k = wh / 1000.0f;
                if (shortFmt || (k >= 100.0f)) snprintf(buf, len, "%.0f kWh", k);
                else if (k >= 10.0f)           snprintf(buf, len, "%.1f kWh", k);
                else                           snprintf(buf, len, "%.2f kWh", k);
            } else
                snprintf(buf, len, "%.0f Wh", wh);
            decimalComma(buf);
        }

        // total yield in kWh -> "999 kWh", "2,84 MWh", "123,4 MWh" (short: no decimals)
        static void formatYieldTotal(char *buf, size_t len, float kwh, bool shortFmt) {
            if (kwh >= 1000.0f) {
                float m = kwh / 1000.0f;
                if (shortFmt || (m >= 1000.0f)) snprintf(buf, len, "%.0f MWh", m);
                else if (m >= 10.0f)            snprintf(buf, len, "%.1f MWh", m);
                else                            snprintf(buf, len, "%.2f MWh", m);
            } else
                snprintf(buf, len, "%.0f kWh", kwh);
            decimalComma(buf);
        }

        //---------------------------------------------------------------------
        // detail page: day curve with peak value, gridline and time axis
        void drawCurvePage(void) {
            int16_t xl = x0();
            uint8_t top = Y_CURVE_DETAIL - (uint8_t)(CURVE_H_DETAIL / 1.1f + 0.5f);

            // gridline at the day maximum, dotted
            if (dayCurveHasData()) {
                for (int16_t x = 0; x < mPgWidth; x += 3)
                    mDisplay->drawPixel(xl + x, top);
            }

            plotDayCurve(xl, Y_CURVE_DETAIL, CURVE_H_DETAIL, 1, true);

            // hour ticks below the baseline (longer at 12:00)
            if (0 != dayCurveStart()) {
                tmElements_t tm;
                breakTime(dayCurveStart(), tm);
                tm.Minute = 0; tm.Second = 0;
                for (uint8_t i = 0; i < 24; i++) {
                    tm.Hour++;
                    if (tm.Hour > 23) break;
                    int16_t px = dayCurveXofTime((uint32_t)makeTime(tm));
                    if (px < 0) break;
                    mDisplay->drawVLine(xl + px, Y_CURVE_DETAIL + 1, (12 == tm.Hour) ? 3 : 1);
                }
            }

            // peak value (right, below status bar)
            mDisplay->setFont(u8g2_font_helvB08_tr);
            if (dayCurveHasData()) {
                float mx = dayCurveMaxPower();
                if (mx >= 1000.0f) snprintf(mFmtText, DISP_FMT_TEXT_LEN, "Max %.2f kW", mx / 1000.0f);
                else               snprintf(mFmtText, DISP_FMT_TEXT_LEN, "Max %.0f W", mx);
                decimalComma(mFmtText);
                int16_t w = mDisplay->getStrWidth(mFmtText);
                drawKnockoutStr(x1() - w, top - 2 > 18 ? top - 2 : 18, mFmtText);
            } else {
                drawKnockoutCentered("Noch keine Daten", xc(), 34);
            }

            // sunrise / sunset at the time axis
            mDisplay->setFont(u8g2_font_4x6_tr);
            if (0 != dayCurveStart()) {
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%02d:%02d", hour(dayCurveStart()), minute(dayCurveStart()));
                mDisplay->drawStr(xl, Y_BOTTOM, mFmtText);
                snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%02d:%02d", hour(dayCurveEnd()), minute(dayCurveEnd()));
                mDisplay->drawStr(x1() - mDisplay->getStrWidth(mFmtText) + 1, Y_BOTTOM, mFmtText);
            }
            mDisplay->setFont(u8g2_font_helvB08_tr);
            float yd = mDisplayData->totalYieldDay;
            if (yd >= 1000.0f) snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%.2f kWh", yd / 1000.0f);
            else               snprintf(mFmtText, DISP_FMT_TEXT_LEN, "%.0f Wh", yd);
            decimalComma(mFmtText);
            mDisplay->setDrawColor(0);
            mDisplay->drawBox(xc() - mDisplay->getStrWidth(mFmtText) / 2 - 2, Y_CURVE_DETAIL + 1, mDisplay->getStrWidth(mFmtText) + 4, 9);
            mDisplay->setDrawColor(1);
            drawCentered(mFmtText, xc(), Y_BOTTOM);
        }

        //---------------------------------------------------------------------
        // helpers
        void knockoutBox(int16_t x, int16_t y, int16_t w, int16_t h) {
            if (x < 0) { w += x; x = 0; }
            if (y < 0) { h += y; y = 0; }
            if ((w <= 0) || (h <= 0)) return;
            mDisplay->setDrawColor(0);
            mDisplay->drawBox(x, y, w, h);
            mDisplay->setDrawColor(1);
        }

        // text with a dark outline (halo), so it stays readable on top of the curve
        void drawHaloStr(int16_t x, int16_t y, const char *txt, uint8_t r = 1) {
            mDisplay->setDrawColor(0);
            for (int8_t dy = -r; dy <= r; dy++)
                for (int8_t dx = -r; dx <= r; dx++)
                    if (dx || dy)
                        mDisplay->drawStr(x + dx, y + dy, txt);
            mDisplay->setDrawColor(1);
            mDisplay->drawStr(x, y, txt);
        }

        void drawHaloXBM(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bmp) {
            mDisplay->setDrawColor(0);
            for (int8_t dy = -2; dy <= 2; dy++)
                for (int8_t dx = -2; dx <= 2; dx++)
                    if (dx || dy)
                        mDisplay->drawXBMP(x + dx, y + dy, w, h, bmp);
            mDisplay->setDrawColor(1);
            mDisplay->drawXBMP(x, y, w, h, bmp);
        }

        void drawKnockoutStr(int16_t x, int16_t y, const char *txt) {
            drawHaloStr(x, y, txt, 1);
        }

        void drawKnockoutCentered(const char *txt, int16_t cx, int16_t y) {
            drawKnockoutStr(cx - mDisplay->getStrWidth(txt) / 2, y, txt);
        }

        void drawCentered(const char *txt, int16_t cx, int16_t y) {
            mDisplay->drawStr(cx - mDisplay->getStrWidth(txt) / 2, y, txt);
        }

        // draws an icon layer either solid or dimmed (every 2nd pixel)
        void drawLevelIcon(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bmp, bool lit) {
            uint8_t bpr = (w + 7) / 8;
            for (uint8_t r = 0; r < h; r++) {
                for (uint8_t c = 0; c < w; c++) {
                    uint8_t b = u8x8_pgm_read(bmp + r * bpr + (c >> 3));
                    if (b & (1 << (c & 7))) {
                        if (lit || (0 == ((r + c) & 1)))
                            mDisplay->drawPixel(x + c, y + r);
                    }
                }
            }
        }

        // German number format: 1.25 -> 1,25
        static void decimalComma(char *s) {
            for (; *s; s++)
                if ('.' == *s) *s = ',';
        }
};
