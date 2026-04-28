#pragma once

#include <TFT_eSPI.h>
#include "CarouselMenu.h"
#include "CLIScreen.h"

enum UIMode {
    UI_CAROUSEL,  // landscape, retro menu
    UI_CLI        // portrait, CLI execution screen
};

// Central controller for the Hybrid UI.
//
// Instantiate once as a global, then:
//   1. Call init() after display setup.
//   2. Call update() every loop iteration.
//   3. Route button presses to handleUp/Down/Center/Back.
//
// showMenu()  → switches to landscape carousel
// showScreen() → switches to portrait CLI screen
class UIManager {
public:
    explicit UIManager(TFT_eSPI& tft);
    ~UIManager();

    void init();    // builds menu cards, sets rotation, first draw
    void update();  // redraws when dirty or animating

    // Button handlers (call from loop)
    void handleUp();
    void handleDown();
    void handleCenter();
    void handleBack();

    // Mode transitions
    void showMenu();
    void showScreen(CLIScreen* screen);

    void markDirty() { _dirty = true; }
    UIMode mode() const { return _mode; }

    // Accessors for pre-allocated screens (used by action stubs in UIManager.cpp)
    WiFiScanScreen* wifiScreen();
    BLEScanScreen*  bleScreen();

private:
    TFT_eSPI&    _tft;
    UIMode       _mode;
    CarouselMenu* _menu;
    CLIScreen*   _screen;   // currently active CLI screen (not owned)
    bool         _dirty;

    // Pre-allocated CLI screens (owned by UIManager)
    WiFiScanScreen* _wifiScreen;
    BLEScanScreen*  _bleScreen;

    void setRotation(int r);
};
