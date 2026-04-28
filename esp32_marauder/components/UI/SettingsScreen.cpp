#include "SettingsScreen.h"
#include "UIManager.h"
#include "../../settings.h"
#include <string.h>

extern Settings  settings_obj;
extern UIManager ui_manager_obj;

// ----------------------------------------------------------------
// Static table of settings items
// ----------------------------------------------------------------

const SettingsScreen::SettingItem SettingsScreen::ITEMS[] = {
    { "UI 방향",    "UILandscape", false },  // toggle: 가로↔세로
    { "PIN 잠금",   "PINEnabled",  false },  // toggle: ON↔OFF
    { "PIN 변경",   "SET_PIN",     true  },  // action: enter PINScreen(SET)
    { "ForcePMKID",         "ForcePMKID",  false },
    { "SavePCAP",           "SavePCAP",    false },
    { "EnableLED",          "EnableLED",   false },
    { "ChanHop",            "ChanHop",     false },
    { "ForceProbe",         "ForceProbe",  false },
    { "EPDeauth",           "EPDeauth",    false },
};
const int SettingsScreen::ITEM_COUNT =
    sizeof(SettingsScreen::ITEMS) / sizeof(SettingsScreen::ITEMS[0]);

// ----------------------------------------------------------------

SettingsScreen::SettingsScreen(TFT_eSPI& tft)
    : CLIScreen(tft, "Settings"),
      _cursor(0), _scrollTop(0)
{}

// ----------------------------------------------------------------
// Navigation
// ----------------------------------------------------------------

void SettingsScreen::handleUp() {
    if (_cursor > 0) {
        _cursor--;
        if (_cursor < _scrollTop) _scrollTop = _cursor;
    }
}

void SettingsScreen::handleDown() {
    if (_cursor < ITEM_COUNT - 1) {
        _cursor++;
        if (_cursor >= _scrollTop + VISIBLE_ROWS)
            _scrollTop = _cursor - VISIBLE_ROWS + 1;
    }
}

// ----------------------------------------------------------------
// Draw
// ----------------------------------------------------------------

const char* SettingsScreen::getValueStr(const SettingItem& item) const {
    if (item.isAction) return ">";

    // UILandscape: show "가로" / "세로"
    if (strcmp(item.key, "UILandscape") == 0) {
        return settings_obj.loadSetting<bool>("UILandscape") ? "가로" : "세로";
    }

    return settings_obj.loadSetting<bool>(item.key) ? "ON" : "OFF";
}

void SettingsScreen::drawItem(int idx, int y, bool selected) {
    const SettingItem& item = ITEMS[idx];

    uint16_t bg  = selected ? GOLD_BORDER : CREAM_BG;
    uint16_t fg  = selected ? DARK_TEXT   : DARK_TEXT;
    uint16_t vfg = selected ? GOLD_PRIMARY : MEDIUM_GRAY;

    _tft.fillRect(0, y, CLI_W, 22, bg);

    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(fg, bg);
    _tft.setCursor(6, y + 7);
    _tft.print(item.label);

    const char* val = getValueStr(item);
    int vw = _tft.textWidth(val);
    _tft.setTextColor(vfg, bg);
    _tft.setCursor(CLI_W - vw - 6, y + 7);
    _tft.print(val);

    // Separator line
    _tft.drawFastHLine(0, y + 21, CLI_W, GOLD_BORDER);
}

void SettingsScreen::draw() {
    _tft.fillScreen(CREAM_BG);
    drawHeader();

    int y = CLI_HEADER_H + 2;
    for (int i = 0; i < VISIBLE_ROWS && (_scrollTop + i) < ITEM_COUNT; i++) {
        int idx = _scrollTop + i;
        drawItem(idx, y, idx == _cursor);
        y += 22;
    }

    drawFooter("C:토글  D:하단  U:상단");
}

// ----------------------------------------------------------------
// Actions
// ----------------------------------------------------------------

void SettingsScreen::onAction() {
    if (_cursor >= ITEM_COUNT) return;
    const SettingItem& item = ITEMS[_cursor];

    if (item.isAction) {
        // Special actions
        if (strcmp(item.key, "SET_PIN") == 0) {
            ui_manager_obj.startPINSetup();
        }
        return;
    }

    // Toggle bool setting
    settings_obj.toggleSetting(item.key);

    // If orientation changed, notify UIManager
    if (strcmp(item.key, "UILandscape") == 0) {
        ui_manager_obj.applyOrientation();
    }
}

void SettingsScreen::onBack() {
    ui_manager_obj.showMenu();
}
