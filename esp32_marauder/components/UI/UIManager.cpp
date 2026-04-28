#include "UIManager.h"
#include "CLIScreen.h"
#include "PINScreen.h"
#include "SettingsScreen.h"
#include "../../settings.h"

extern Settings settings_obj;

// ----------------------------------------------------------------
// Static back-reference so plain function pointers can reach the
// UIManager without captures (MenuCard::action is void(*)()).
// ----------------------------------------------------------------
static UIManager* g_ui = nullptr;

// ----------------------------------------------------------------
// MenuCard action stubs
// ----------------------------------------------------------------
static void actionWifi() {
    if (g_ui) g_ui->showScreen(g_ui->wifiScreen());
}
static void actionBLE() {
    if (g_ui) g_ui->showScreen(g_ui->bleScreen());
}
static void actionDeauth() {
    if (g_ui) g_ui->showScreen(g_ui->deauthScreen());
}
static void actionLock() {
    if (g_ui) g_ui->lockScreen();
}
static void actionSettings() {
    if (g_ui) g_ui->showScreen(g_ui->settingsScreen());
}

// ----------------------------------------------------------------
// Static menu card table
// ----------------------------------------------------------------
static MenuCard s_cards[] = {
    { "WiFi Scan",   "Scan APs",      actionWifi     },
    { "BLE Scan",    "Scan devices",  actionBLE      },
    { "Deauth Snif", "Sniff packets", actionDeauth   },
    { "Lock",        "Lock device",   actionLock     },
    { "Settings",    "Configure",     actionSettings },
};
static const int CARD_COUNT = sizeof(s_cards) / sizeof(s_cards[0]);

// ----------------------------------------------------------------
// UIManager implementation
// ----------------------------------------------------------------

UIManager::UIManager(TFT_eSPI& tft)
    : _tft(tft),
      _mode(UI_CAROUSEL),
      _menu(nullptr),
      _screen(nullptr),
      _dirty(true),
      _locked(false),
      _portrait(false),
      _wifiScreen(nullptr),
      _bleScreen(nullptr),
      _deauthScreen(nullptr),
      _settingsScreen(nullptr),
      _pinScreen(nullptr)
{}

UIManager::~UIManager() {
    delete _menu;
    delete _wifiScreen;
    delete _bleScreen;
    delete _deauthScreen;
    delete _settingsScreen;
    delete _pinScreen;
}

// Screen accessors
WiFiScanScreen*  UIManager::wifiScreen()    { return _wifiScreen;    }
BLEScanScreen*   UIManager::bleScreen()     { return _bleScreen;     }
DeauthScanScreen* UIManager::deauthScreen() { return _deauthScreen;  }
SettingsScreen*  UIManager::settingsScreen(){ return _settingsScreen; }
PINScreen*       UIManager::pinScreen()     { return _pinScreen;     }

void UIManager::init() {
    g_ui = this;

    _menu           = new CarouselMenu(s_cards, CARD_COUNT);
    _wifiScreen     = new WiFiScanScreen(_tft);
    _bleScreen      = new BLEScanScreen(_tft);
    _deauthScreen   = new DeauthScanScreen(_tft);
    _settingsScreen = new SettingsScreen(_tft);
    _pinScreen      = new PINScreen(_tft);

    applyOrientation();
    showMenu();
}

void UIManager::setRotation(int r) {
    _tft.setRotation(r);
}

// ----------------------------------------------------------------
// Orientation
// ----------------------------------------------------------------

void UIManager::applyOrientation() {
    bool landscape = settings_obj.loadSetting<bool>("UILandscape");
    _portrait = !landscape;
    if (_menu) _menu->setPortraitMode(_portrait);
}

// ----------------------------------------------------------------
// Mode transitions
// ----------------------------------------------------------------

void UIManager::showMenu() {
    // PIN screen always stays in portrait (rotation 0).
    // Menu uses landscape (rot 1) or portrait (rot 0) based on setting.
    setRotation(_portrait ? 0 : 1);
    _mode   = UI_CAROUSEL;
    _screen = nullptr;
    markDirty();
}

void UIManager::showScreen(CLIScreen* screen) {
    setRotation(0);  // CLI screens always portrait
    _mode   = UI_CLI;
    _screen = screen;
    markDirty();
}

// ----------------------------------------------------------------
// Lock / PIN
// ----------------------------------------------------------------

void UIManager::lockScreen() {
    _locked = true;
    _pinScreen->setMode(PIN_VERIFY);
    _pinScreen->setCallback(
        // onSuccess: unlock and return to menu
        []() {
            if (g_ui) {
                g_ui->_locked = false;
                g_ui->showMenu();
            }
        },
        nullptr  // onFail = nullptr → cancel disabled (lock screen)
    );
    showScreen(_pinScreen);
}

void UIManager::requestPIN(void (*onSuccess)(), void (*onFail)()) {
    _pinScreen->setMode(PIN_VERIFY);
    _pinScreen->setCallback(onSuccess, onFail);
    showScreen(_pinScreen);
}

void UIManager::startPINSetup() {
    _pinScreen->setMode(PIN_SET);
    _pinScreen->setCallback(
        // onSuccess: PIN saved, return to settings
        []() { if (g_ui) g_ui->showScreen(g_ui->settingsScreen()); },
        // onCancel: back to settings
        []() { if (g_ui) g_ui->showScreen(g_ui->settingsScreen()); }
    );
    showScreen(_pinScreen);
}

// ----------------------------------------------------------------
// Update (render loop)
// ----------------------------------------------------------------

void UIManager::update() {
    if (_mode == UI_CAROUSEL) {
        if (_menu->isAnimating() || _dirty) {
            _menu->draw(_tft);
            _dirty = false;
        }
    } else {
        if (_screen) {
            _screen->update();
            if (_dirty) {
                _screen->draw();
                _dirty = false;
            }
        }
    }
}

// ----------------------------------------------------------------
// Button handlers
// ----------------------------------------------------------------

void UIManager::handleUp() {
    if (_mode == UI_CAROUSEL) {
        _menu->selectPrev();
    } else if (_screen == _pinScreen) {
        _pinScreen->handleDigitPrev();
        markDirty();
    } else if (_screen == _settingsScreen) {
        _settingsScreen->handleUp();
        markDirty();
    }
}

void UIManager::handleDown() {
    if (_mode == UI_CAROUSEL) {
        _menu->selectNext();
    } else if (_screen == _pinScreen) {
        _pinScreen->handleDigitNext();
        markDirty();
    } else if (_screen == _settingsScreen) {
        _settingsScreen->handleDown();
        markDirty();
    }
}

void UIManager::handleCenter() {
    if (_mode == UI_CAROUSEL) {
        _menu->executeSelected();
    } else if (_screen) {
        _screen->onAction();
        markDirty();
    }
}

void UIManager::handleBack() {
    if (_mode == UI_CLI) {
        if (_screen) _screen->onBack();
        // If screen's onBack() calls showMenu() itself (e.g. SettingsScreen),
        // we don't double-switch; only fall through when screen didn't change.
        if (_mode == UI_CLI) showMenu();
    }
    // In lock state ignore back (PIN screen's onBack handles it)
}
