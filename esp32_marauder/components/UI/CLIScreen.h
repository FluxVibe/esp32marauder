#pragma once

#include <TFT_eSPI.h>
#include "Theme.h"
#include "Widgets.h"

// ----------------------------------------------------------------
// Base class for all CLI-style portrait screens.
// Subclasses implement draw() and optionally update()/onAction()/onBack().
// ----------------------------------------------------------------
class CLIScreen {
public:
    CLIScreen(TFT_eSPI& tft, const char* title);
    virtual ~CLIScreen() {}

    virtual void draw()     = 0;  // full repaint
    virtual void update()   {}    // called every loop tick for live data
    virtual void onAction() {}    // center/action button
    virtual void onBack()   {}    // back button (before UIManager switches mode)

protected:
    TFT_eSPI&   _tft;
    const char* _title;

    // Draws the top header bar (20px tall).
    // If actionLabel is non-null it is right-aligned in the bar.
    void drawHeader(const char* actionLabel = nullptr);

    // Draws a one-line footer at CLI_FOOTER_Y.
    void drawFooter(const char* text);
};

// ----------------------------------------------------------------
// WiFi AP scan screen
// ----------------------------------------------------------------
class WiFiScanScreen : public CLIScreen {
public:
    WiFiScanScreen(TFT_eSPI& tft);

    void draw()     override;
    void update()   override;
    void onAction() override;
    void onBack()   override;

    // Called by UIManager / wifi_scan_obj callbacks to push results
    void addAP(const char* ssid, int rssi);
    void setProgress(int pct);
    void setScanning(bool scanning);

private:
    static const int MAX_AP = 12;

    struct APEntry {
        char ssid[33];
        int  rssi;
    };

    APEntry _aps[MAX_AP];
    int     _apCount;
    int     _progress;      // 0-100
    bool    _scanning;
    unsigned long _startMs;

    void drawAPList();
};

// ----------------------------------------------------------------
// BLE device scan screen
// ----------------------------------------------------------------
class BLEScanScreen : public CLIScreen {
public:
    BLEScanScreen(TFT_eSPI& tft);

    void draw()     override;
    void update()   override;
    void onAction() override;
    void onBack()   override;

    void addDevice(const char* name, int rssi);

private:
    static const int MAX_DEV = 12;

    struct DevEntry {
        char name[32];
        int  rssi;
    };

    DevEntry _devs[MAX_DEV];
    int      _devCount;
    bool     _scanning;
    unsigned long _startMs;
};
