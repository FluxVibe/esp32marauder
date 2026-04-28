#include "UIManager.h"

// ----------------------------------------------------------------
// Static back-reference so plain function pointers can reach the
// UIManager without captures (MenuCard::action is void(*)()).
// ----------------------------------------------------------------
static UIManager* g_ui = nullptr;

// ----------------------------------------------------------------
// MenuCard action stubs — switch to the appropriate CLI screen
// ----------------------------------------------------------------
static void actionWifi() {
    if (g_ui) g_ui->showScreen(g_ui->wifiScreen());
}
static void actionBLE() {
    if (g_ui) g_ui->showScreen(g_ui->bleScreen());
}
static void actionDeauth() {
    // Reuse the WiFi scan screen but in deauth-sniff context (future)
    if (g_ui) g_ui->showScreen(g_ui->wifiScreen());
}
static void actionSettings() {
    // Placeholder — navigate back until a settings screen is implemented
    if (g_ui) g_ui->showMenu();
}

// ----------------------------------------------------------------
// Static menu card table
// ----------------------------------------------------------------
static MenuCard s_cards[] = {
    { "WiFi Scan",   "Scan APs",       actionWifi     },
    { "BLE Scan",    "Scan devices",   actionBLE      },
    { "Deauth Snif", "Sniff packets",  actionDeauth   },
    { "Settings",    "Configure",      actionSettings },
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
      _wifiScreen(nullptr),
      _bleScreen(nullptr)
{}

UIManager::~UIManager() {
    delete _menu;
    delete _wifiScreen;
    delete _bleScreen;
}

// Access helpers used by action stubs above
WiFiScanScreen* UIManager::wifiScreen() { return _wifiScreen; }
BLEScanScreen*  UIManager::bleScreen()  { return _bleScreen;  }

void UIManager::init() {
    g_ui = this;

    _menu       = new CarouselMenu(s_cards, CARD_COUNT);
    _wifiScreen = new WiFiScanScreen(_tft);
    _bleScreen  = new BLEScanScreen(_tft);

    showMenu();
}

void UIManager::setRotation(int r) {
    _tft.setRotation(r);
}

void UIManager::showMenu() {
    setRotation(1);          // landscape
    _mode   = UI_CAROUSEL;
    _screen = nullptr;
    markDirty();
}

void UIManager::showScreen(CLIScreen* screen) {
    setRotation(0);          // portrait
    _mode   = UI_CLI;
    _screen = screen;
    markDirty();
}

void UIManager::update() {
    if (_mode == UI_CAROUSEL) {
        // Always redraw while animating; otherwise only when dirty
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
    }
}

void UIManager::handleDown() {
    if (_mode == UI_CAROUSEL) {
        _menu->selectNext();
    }
}

void UIManager::handleCenter() {
    if (_mode == UI_CAROUSEL) {
        _menu->executeSelected();  // calls the card's action → showScreen()
    } else if (_screen) {
        _screen->onAction();
        markDirty();
    }
}

void UIManager::handleBack() {
    if (_mode == UI_CLI) {
        if (_screen) _screen->onBack();
        showMenu();
    }
}
