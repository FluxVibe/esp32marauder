#pragma once

#include <TFT_eSPI.h>
#include "Theme.h"

namespace Widgets {

// Horizontal progress bar.
// x,y = top-left corner; w = width; h = height; pct = 0..100
void drawProgressBar(TFT_eSPI& tft, int x, int y, int w, int h, int pct);

// Four vertical bars scaled by rssi strength.
// rssi: 0 = signal not available, negative dBm otherwise (e.g. -45 is strong)
// x,y = top-left of 16x12 bounding box
void drawSignalBars(TFT_eSPI& tft, int x, int y, int rssi);

// One AP list row: "#N  <ssid>  <rssi>dBm"
// x,y = left edge of the row baseline; maxW = available pixel width
void drawAPRow(TFT_eSPI& tft, int x, int y,
               int index, const char* ssid, int rssi);

// Simple key=value row in CLI body (e.g. "Channel : 6")
void drawKeyValue(TFT_eSPI& tft, int x, int y,
                  const char* key, const char* value);

} // namespace Widgets
