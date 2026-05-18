#pragma once

#include "CLIScreen.h"

// Portrait settings list screen.
//
// Shows a scrollable list of settings items.
// Navigation:
//   D_BTN → handleDown() — move cursor down
//   U_BTN → handleUp()   — move cursor up
//   C_BTN → onAction()   — toggle bool setting, or enter action
//   Back  → onBack()     — return to carousel menu
class SettingsScreen : public CLIScreen {
public:
    explicit SettingsScreen(TFT_eSPI& tft);

    void draw()     override;
    void onAction() override;
    void onBack()   override;

    // Routed from UIManager when this screen is active
    void handleUp();
    void handleDown();

private:
    struct SettingItem {
        const char* label;
        const char* key;      // settings_obj key, or "SET_PIN" special action
        bool        isAction; // true = execute (not toggle)
        bool        isCycle;  // true = 3-way String cycle (Low/Medium/High)
    };

    static const SettingItem ITEMS[];
    static const int         ITEM_COUNT;

    int _cursor;    // currently highlighted row
    int _scrollTop; // first visible row index

    // computed so rows fit between header and footer on any display size
    static const int VISIBLE_ROWS = (CLI_FOOTER_Y - CLI_HEADER_H) / 22;

    void drawItem(int idx, int y, bool selected);
    const char* getValueStr(const SettingItem& item) const;
};
