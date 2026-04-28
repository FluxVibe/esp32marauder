#include "Widgets.h"
#include <stdio.h>

namespace Widgets {

void drawProgressBar(TFT_eSPI& tft, int x, int y, int w, int h, int pct) {
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    int filled = (w * pct) / 100;

    // Background track
    tft.fillRect(x, y, w, h, GOLD_BORDER);

    // Filled portion
    if (filled > 0)
        tft.fillRect(x, y, filled, h, GOLD_PRIMARY);

    // Outline
    tft.drawRect(x, y, w, h, MEDIUM_GRAY);
}

void drawSignalBars(TFT_eSPI& tft, int x, int y, int rssi) {
    // 4 bars, each 3px wide, 2px gap, heights 3,5,8,11
    const int barW  = 3;
    const int gap   = 2;
    const int heights[4] = { 3, 5, 8, 11 };

    // Determine how many bars to fill based on rssi
    int bars = 0;
    if      (rssi >= -55) bars = 4;
    else if (rssi >= -67) bars = 3;
    else if (rssi >= -78) bars = 2;
    else if (rssi <  0)   bars = 1;

    for (int i = 0; i < 4; i++) {
        int bx = x + i * (barW + gap);
        int bh = heights[i];
        int by = y + (11 - bh);  // align to bottom
        uint16_t col = (i < bars) ? GOLD_PRIMARY : GOLD_BORDER;
        tft.fillRect(bx, by, barW, bh, col);
        tft.drawRect(bx, by, barW, bh, MEDIUM_GRAY);
    }
}

void drawAPRow(TFT_eSPI& tft, int x, int y,
               int index, const char* ssid, int rssi) {
    char buf[48];
    snprintf(buf, sizeof(buf), "#%-2d %-16s %ddBm", index + 1, ssid, rssi);

    tft.setTextFont(FONT_CLI_BODY);
    tft.setTextColor(DARK_TEXT, CREAM_BG);
    tft.setCursor(x, y);
    tft.print(buf);
}

void drawKeyValue(TFT_eSPI& tft, int x, int y,
                  const char* key, const char* value) {
    tft.setTextFont(FONT_CLI_BODY);

    tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    tft.setCursor(x, y);
    tft.print(key);
    tft.print(": ");

    tft.setTextColor(DARK_TEXT, CREAM_BG);
    tft.print(value);
}

} // namespace Widgets
