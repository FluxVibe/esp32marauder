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
    { "UI Orient", "UILandscape",  false, false },  // toggle: Landscape/Portrait
    { "PIN Lock",  "PINEnabled",   false, false },  // toggle: ON/OFF
    { "Set PIN",   "SET_PIN",      true,  false },  // action: enter PINScreen(SET)
    { "WiFi Power","WiFiTxPower",  false, true  },  // cycle: Low/Medium/High
    { "BLE Power", "BTTxPower",    false, true  },  // cycle: Low/Medium/High
    { "ForcePMKID","ForcePMKID",   false, false },
    { "SavePCAP",  "SavePCAP",     false, false },
    { "EncryptPCAP","EncryptPCAP", false, false },
    { "EnableLED", "EnableLED",    false, false },
    { "ChanHop",   "ChanHop",      false, false },
    { "ForceProbe","ForceProbe",   false, false },
    { "EPDeauth",  "EPDeauth",     false, false },
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

    if (item.isCycle) {
        // 3-way String setting: return stored value directly
        static char buf[8];
        String v = settings_obj.loadSetting<String>(item.key);
        strncpy(buf, v.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }

    // UILandscape: show Landscape / Portrait
    if (strcmp(item.key, "UILandscape") == 0) {
        return settings_obj.loadSetting<bool>("UILandscape") ? "Landscape" : "Portrait";
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

    drawFooter("C:Change D:Down U:Up");
}

// ----------------------------------------------------------------
// Actions
// ----------------------------------------------------------------

void SettingsScreen::onAction() {
    if (_cursor >= ITEM_COUNT) return;
    const SettingItem& item = ITEMS[_cursor];

    if (item.isAction) {
        if (strcmp(item.key, "SET_PIN") == 0) {
            ui_manager_obj.startPINSetup();
        }
        return;
    }

    if (item.isCycle) {
        // Cycle Low → Medium → High → Low
        String cur = settings_obj.loadSetting<String>(item.key);
        String next;
        if (cur == "Low")        next = "Medium";
        else if (cur == "Medium") next = "High";
        else                      next = "Low";
        settings_obj.saveSetting<String>(item.key, next);
        ui_manager_obj.markDirty();
        return;
    }

    // Toggle bool setting
    settings_obj.toggleSetting(item.key);

    // If orientation changed, notify UIManager
    if (strcmp(item.key, "UILandscape") == 0) {
        ui_manager_obj.applyOrientation();
    }
    ui_manager_obj.markDirty();
}

void SettingsScreen::onBack() {
    ui_manager_obj.showMenu();
}
