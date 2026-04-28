#include "CLIScreen.h"
#include "UIManager.h"
#include "../../WiFiScan.h"
#include "../../settings.h"
#include <stdio.h>
#include <string.h>

extern WiFiScan  wifi_scan_obj;
extern Settings  settings_obj;
extern UIManager ui_manager_obj;

// ================================================================
//  CLIScreen base
// ================================================================

CLIScreen::CLIScreen(TFT_eSPI& tft, const char* title)
    : _tft(tft), _title(title) {}

void CLIScreen::drawHeader(const char* actionLabel) {
    // Solid header bar
    _tft.fillRect(0, 0, CLI_W, CLI_HEADER_H, GOLD_BORDER);

    // Back arrow
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(DARK_TEXT, GOLD_BORDER);
    _tft.setCursor(3, 6);
    _tft.print("<");

    // Title
    _tft.setCursor(14, 6);
    _tft.print(_title);

    // Optional action label (right-aligned)
    if (actionLabel) {
        int tw = _tft.textWidth(actionLabel);
        _tft.setCursor(CLI_W - tw - 4, 6);
        _tft.print(actionLabel);
    }
}

void CLIScreen::drawFooter(const char* text) {
    _tft.fillRect(0, CLI_FOOTER_Y - 1, CLI_W, CLI_H - CLI_FOOTER_Y + 1, GOLD_BORDER);
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(DARK_TEXT, GOLD_BORDER);
    _tft.setCursor(4, CLI_FOOTER_Y + 2);
    _tft.print(text);
}

// ================================================================
//  WiFiScanScreen
// ================================================================

WiFiScanScreen::WiFiScanScreen(TFT_eSPI& tft)
    : CLIScreen(tft, "WiFi Scan"),
      _apCount(0), _progress(0), _scanning(false), _startMs(0)
{
    memset(_aps, 0, sizeof(_aps));
}

void WiFiScanScreen::setProgress(int pct) {
    _progress = pct;
}

void WiFiScanScreen::setScanning(bool scanning) {
    _scanning = scanning;
    if (scanning) _startMs = millis();
}

void WiFiScanScreen::addAP(const char* ssid, int rssi) {
    if (_apCount >= MAX_AP) return;
    strncpy(_aps[_apCount].ssid, ssid, 32);
    _aps[_apCount].ssid[32] = '\0';
    _aps[_apCount].rssi = rssi;
    _apCount++;
}

void WiFiScanScreen::draw() {
    _tft.fillScreen(CREAM_BG);
    drawHeader(_scanning ? "Stop" : "Scan");

    int y = CLI_BODY_START + 2;

    // Prompt line
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    _tft.setCursor(4, y);
    _tft.print("solinlab@marauder");
    y += 14;

    // Status line
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(DARK_TEXT, CREAM_BG);
    _tft.setCursor(4, y);
    _tft.print(_scanning ? "Scanning..." : (_apCount > 0 ? "Scan complete." : "Press [Scan]"));
    y += 14;

    // Progress bar (only while scanning)
    if (_scanning) {
        Widgets::drawProgressBar(_tft, 4, y, CLI_W - 8, 10, _progress);
        y += 16;
    }

    // AP list
    drawAPList();

    // Footer
    char footer[32];
    unsigned long elapsed = _scanning ? (millis() - _startMs) / 1000 : 0;
    snprintf(footer, sizeof(footer), "APs: %d  |  %lus", _apCount, elapsed);
    drawFooter(footer);
}

void WiFiScanScreen::drawAPList() {
    int y = CLI_BODY_START + 46;  // below prompt+status+progress
    if (!_scanning) y -= 16;

    for (int i = 0; i < _apCount && y < CLI_FOOTER_Y - 14; i++) {
        Widgets::drawAPRow(_tft, 4, y, i, _aps[i].ssid, _aps[i].rssi);
        y += 15;
    }
}

void WiFiScanScreen::update() {
    // Sync progress from wifi_scan_obj (called every loop tick)
    // The WiFiScan object does not expose scan progress directly; we
    // approximate it via elapsed time (AP scan takes ~5 s default).
    if (_scanning) {
        unsigned long elapsed = millis() - _startMs;
        int pct = (int)(elapsed / 50);  // 5000 ms → 100 %
        if (pct > 99) pct = 99;
        _progress = pct;
    }
}

void WiFiScanScreen::onAction() {
    if (_scanning) {
        // Stop scan
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        _scanning = false;
        _progress = 100;
    } else {
        // Start scan — clear previous results
        _apCount  = 0;
        _progress = 0;
        memset(_aps, 0, sizeof(_aps));
        setScanning(true);
        wifi_scan_obj.StartScan(WIFI_SCAN_AP);
    }
}

void WiFiScanScreen::onBack() {
    if (_scanning) {
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        _scanning = false;
    }
}

// ================================================================
//  BLEScanScreen
// ================================================================

BLEScanScreen::BLEScanScreen(TFT_eSPI& tft)
    : CLIScreen(tft, "BLE Scan"),
      _devCount(0), _scanning(false), _startMs(0)
{
    memset(_devs, 0, sizeof(_devs));
}

void BLEScanScreen::addDevice(const char* name, int rssi) {
    if (_devCount >= MAX_DEV) return;
    strncpy(_devs[_devCount].name, name, 31);
    _devs[_devCount].name[31] = '\0';
    _devs[_devCount].rssi = rssi;
    _devCount++;
}

void BLEScanScreen::draw() {
    _tft.fillScreen(CREAM_BG);
    drawHeader(_scanning ? "Stop" : "Scan");

    int y = CLI_BODY_START + 2;

    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    _tft.setCursor(4, y);
    _tft.print("solinlab@marauder");
    y += 14;

    _tft.setTextColor(DARK_TEXT, CREAM_BG);
    _tft.setCursor(4, y);
    _tft.print(_scanning ? "BLE scanning..." : (_devCount > 0 ? "Scan complete." : "Press [Scan]"));
    y += 14;

    for (int i = 0; i < _devCount && y < CLI_FOOTER_Y - 14; i++) {
        Widgets::drawAPRow(_tft, 4, y, i, _devs[i].name, _devs[i].rssi);
        y += 15;
    }

    char footer[32];
    snprintf(footer, sizeof(footer), "Devs: %d", _devCount);
    drawFooter(footer);
}

void BLEScanScreen::update() {
    // BLE scan results are pushed via addDevice() from callbacks
}

void BLEScanScreen::onAction() {
    if (_scanning) {
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        _scanning = false;
    } else {
        _devCount = 0;
        memset(_devs, 0, sizeof(_devs));
        _scanning = true;
        _startMs  = millis();
        wifi_scan_obj.StartScan(BT_SCAN_ALL);
    }
}

void BLEScanScreen::onBack() {
    if (_scanning) {
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        _scanning = false;
    }
}

// ================================================================
//  DeauthScanScreen  — PIN-gated attack screen
// ================================================================

DeauthScanScreen::DeauthScanScreen(TFT_eSPI& tft)
    : CLIScreen(tft, "Deauth Snif"),
      _scanning(false), _packetCount(0), _startMs(0)
{}

void DeauthScanScreen::draw() {
    _tft.fillScreen(CREAM_BG);
    drawHeader(_scanning ? "Stop" : "Start");

    int y = CLI_BODY_START + 4;

    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    _tft.setCursor(4, y);
    _tft.print("solinlab@marauder");
    y += 14;

    _tft.setTextColor(DARK_TEXT, CREAM_BG);
    _tft.setCursor(4, y);
    if (_scanning) {
        _tft.print("Sniffing deauth...");
    } else {
        _tft.setTextColor(WARNING_ORANGE, CREAM_BG);
        _tft.print("! Attack function");
        y += 12;
        _tft.setTextColor(DARK_TEXT, CREAM_BG);
        _tft.setCursor(4, y);
        _tft.print("PIN required to start");
    }
    y += 16;

    if (_scanning) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Packets: %d", _packetCount);
        _tft.setTextFont(FONT_CLI_BODY);
        _tft.setTextColor(DARK_TEXT, CREAM_BG);
        _tft.setCursor(4, y);
        _tft.print(buf);
    }

    char footer[32];
    if (_scanning) {
        unsigned long s = (millis() - _startMs) / 1000;
        snprintf(footer, sizeof(footer), "Time: %lus", s);
    } else {
        snprintf(footer, sizeof(footer), "C:Start(PIN)  Back:Exit");
    }
    drawFooter(footer);
}

void DeauthScanScreen::update() {
    if (_scanning) {
        // Packet count would be updated via wifi_scan_obj callbacks in a full
        // implementation; for now we just mark dirty for elapsed time refresh.
        ui_manager_obj.markDirty();
    }
}

void DeauthScanScreen::onAction() {
    if (_scanning) {
        // Stop
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        _scanning = false;
        ui_manager_obj.markDirty();
        return;
    }

    // Check PIN gate
    bool pinEnabled = settings_obj.loadSetting<bool>("PINEnabled");
    if (pinEnabled) {
        ui_manager_obj.requestPIN(
            // onSuccess: start the actual sniff
            []() { ui_manager_obj.deauthScreen()->doStart(); },
            // onFail / cancel: go back to menu
            []() { ui_manager_obj.showMenu(); }
        );
    } else {
        doStart();
    }
}

void DeauthScanScreen::doStart() {
    _packetCount = 0;
    _scanning    = true;
    _startMs     = millis();
    wifi_scan_obj.StartScan(WIFI_SCAN_DEAUTH, TFT_RED);
    ui_manager_obj.markDirty();
}

void DeauthScanScreen::onBack() {
    if (_scanning) {
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        _scanning = false;
    }
}
