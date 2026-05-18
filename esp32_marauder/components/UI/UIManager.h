#pragma once

#include <TFT_eSPI.h>
#include "CarouselMenu.h"

// Forward declarations — full headers included in UIManager.cpp
class CLIScreen;
class WiFiScanScreen;
class BLEScanScreen;
class DeauthScanScreen;
class PINScreen;
class SettingsScreen;
#ifdef HAS_AUDIO
class MusicScreen;
#endif

enum UIMode {
    UI_CAROUSEL,  // landscape or portrait, retro menu
    UI_CLI        // portrait, CLI execution screen
};

// Central controller for the Hybrid UI.
//
// Instantiate once as a global, then:
//   1. Call init() after display setup.
//   2. Call update() every loop iteration.
//   3. Route button presses to handleUp/Down/Center/Back.
class UIManager {
public:
    explicit UIManager(TFT_eSPI& tft);
    ~UIManager();

    void init();
    void update();

    // Button handlers — call from loop()
    void handleUp();
    void handleDown();
    void handleCenter();
    void handleCenterHold();  // C long-press: volume/EQ overlay in MusicScreen
    void handleBack();

    // Mode transitions
    void showMenu();
    void showScreen(CLIScreen* screen);

    // Orientation (reads UILandscape from settings, applies rotation + carousel mode)
    void applyOrientation();

    // Lock / PIN
    void lockScreen();
    void requestPIN(void (*onSuccess)(), void (*onFail)() = nullptr);
    void startPINSetup();   // called by SettingsScreen
    bool isLocked() const { return _locked; }

    void markDirty() { _dirty = true; }
    UIMode mode() const { return _mode; }

    // Screen accessors
    WiFiScanScreen*   wifiScreen();
    BLEScanScreen*    bleScreen();
    DeauthScanScreen* deauthScreen();
    SettingsScreen*   settingsScreen();
    PINScreen*        pinScreen();
#ifdef HAS_AUDIO
    MusicScreen*      musicScreen();
    void              showMusicScreen();
#endif

private:
    TFT_eSPI&      _tft;
    UIMode         _mode;
    CarouselMenu*  _menu;
    CLIScreen*     _screen;   // currently active CLI screen (not owned)
    bool           _dirty;
    bool           _locked;
    bool           _portrait; // cached orientation state

    // Owned screens
    WiFiScanScreen*   _wifiScreen;
    BLEScanScreen*    _bleScreen;
    DeauthScanScreen* _deauthScreen;
    SettingsScreen*   _settingsScreen;
    PINScreen*        _pinScreen;
#ifdef HAS_AUDIO
    MusicScreen*      _musicScreen;
#endif

    void setRotation(int r);
};
