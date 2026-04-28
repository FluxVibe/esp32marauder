#include "PINScreen.h"
#include "../../settings.h"
#include <string.h>
#include <stdio.h>

extern Settings settings_obj;

// Forward declaration — UIManager's g_ui pointer used only by callbacks
// that are set at runtime, so no header dependency needed here.

PINScreen::PINScreen(TFT_eSPI& tft)
    : CLIScreen(tft, "PIN"),
      _mode(PIN_VERIFY),
      _pos(0), _curDigit(0), _showError(false),
      _onSuccess(nullptr), _onFail(nullptr)
{
    memset(_digits, 0, sizeof(_digits));
}

void PINScreen::setMode(PINMode mode) {
    _mode = mode;
    reset();
}

void PINScreen::setCallback(void (*onSuccess)(), void (*onFail)()) {
    _onSuccess = onSuccess;
    _onFail    = onFail;
}

void PINScreen::reset() {
    _pos       = 0;
    _curDigit  = 0;
    _showError = false;
    memset(_digits, 0, sizeof(_digits));
}

// ----------------------------------------------------------------
// Draw
// ----------------------------------------------------------------

void PINScreen::draw() {
    _tft.fillScreen(CREAM_BG);

    const char* headerTitle = (_mode == PIN_VERIFY) ? "PIN 필요" : "PIN 설정";
    drawHeader(nullptr);

    // Override header title
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(DARK_TEXT, GOLD_BORDER);
    _tft.setCursor(14, 6);
    _tft.print(headerTitle);

    drawSlots();
    drawDigitSelector();
    drawFooter("D:+1  C:확인  C길게:취소");
}

void PINScreen::drawSlots() {
    // 6 slots arranged horizontally, centred in 135px width
    const int slotW  = 16;
    const int slotH  = 22;
    const int gap    = 4;
    const int totalW = PIN_LEN * slotW + (PIN_LEN - 1) * gap;
    int startX = (CLI_W - totalW) / 2;
    int y      = CLI_BODY_START + 10;

    for (int i = 0; i < PIN_LEN; i++) {
        int x = startX + i * (slotW + gap);

        uint16_t borderColor;
        if (_showError) {
            borderColor = ERROR_RED;
        } else if (i == _pos) {
            borderColor = GOLD_PRIMARY;  // active slot
        } else if (i < _pos) {
            borderColor = SUCCESS_GREEN; // confirmed
        } else {
            borderColor = MEDIUM_GRAY;   // pending
        }

        _tft.drawRect(x, y, slotW, slotH, borderColor);

        _tft.setTextFont(FONT_MEDIUM);
        _tft.setTextColor(borderColor, CREAM_BG);
        int tx = x + (slotW - 6) / 2;
        int ty = y + (slotH - 14) / 2;
        _tft.setCursor(tx, ty);
        if (i < _pos) {
            _tft.print('*');
        } else if (i == _pos) {
            _tft.print((char)('0' + _curDigit));
        } else {
            _tft.print('_');
        }
    }
}

void PINScreen::drawDigitSelector() {
    // Large centred digit with prev/next hints
    int cx = CLI_W / 2;
    int cy = CLI_BODY_START + 80;

    // Previous digit (small, gray)
    int prev = (_curDigit + 9) % 10;
    _tft.setTextFont(FONT_SMALL);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", prev);
    int pw = _tft.textWidth(buf);
    _tft.setCursor(cx - pw / 2, cy - 30);
    _tft.print(buf);

    // Current digit (large, gold)
    _tft.setTextFont(FONT_LARGE);
    _tft.setTextColor(GOLD_PRIMARY, CREAM_BG);
    snprintf(buf, sizeof(buf), "%d", _curDigit);
    int cw = _tft.textWidth(buf);
    _tft.setCursor(cx - cw / 2, cy - 14);
    _tft.print(buf);

    // Next digit (small, gray)
    int next = (_curDigit + 1) % 10;
    _tft.setTextFont(FONT_SMALL);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    snprintf(buf, sizeof(buf), "%d", next);
    int nw = _tft.textWidth(buf);
    _tft.setCursor(cx - nw / 2, cy + 22);
    _tft.print(buf);

    // Position label
    _tft.setTextFont(FONT_SMALL);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    char pos_buf[16];
    snprintf(pos_buf, sizeof(pos_buf), "%d / %d", _pos + 1, PIN_LEN);
    int lw = _tft.textWidth(pos_buf);
    _tft.setCursor(cx - lw / 2, cy + 40);
    _tft.print(pos_buf);
}

// ----------------------------------------------------------------
// Button handlers (called by UIManager)
// ----------------------------------------------------------------

void PINScreen::handleDigitNext() {
    _curDigit = (_curDigit + 1) % 10;
}

void PINScreen::handleDigitPrev() {
    _curDigit = (_curDigit + 9) % 10;
}

void PINScreen::handleConfirm() {
    if (_pos >= PIN_LEN) return;

    _digits[_pos] = _curDigit;
    _pos++;
    _curDigit = 0;

    if (_pos == PIN_LEN) {
        if (_mode == PIN_VERIFY)
            verify();
        else
            saveNew();
    }
}

void PINScreen::onAction() {
    handleConfirm();
}

void PINScreen::onBack() {
    if (_onFail) _onFail();
    // If _onFail is nullptr this is the lock screen — ignore back
}

void PINScreen::update() {}

// ----------------------------------------------------------------
// Verification / save
// ----------------------------------------------------------------

void PINScreen::buildEnteredString(char* buf) const {
    for (int i = 0; i < PIN_LEN; i++)
        buf[i] = '0' + _digits[i];
    buf[PIN_LEN] = '\0';
}

void PINScreen::verify() {
    char entered[PIN_LEN + 1];
    buildEnteredString(entered);

    String stored = settings_obj.loadSetting<String>("PINCode");

    if (String(entered) == stored) {
        if (_onSuccess) _onSuccess();
    } else {
        // Flash error state
        _showError = true;
        draw();
        delay(800);
        reset();
        if (_onFail) _onFail();
        // Stay on PIN screen if _onFail == nullptr (lock mode: just reset)
    }
}

void PINScreen::saveNew() {
    char entered[PIN_LEN + 1];
    buildEnteredString(entered);

    settings_obj.saveSetting<String>("PINCode", String(entered));
    settings_obj.saveSetting<bool>("PINEnabled", true);

    // Brief confirmation
    _tft.fillScreen(CREAM_BG);
    drawHeader(nullptr);
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(DARK_TEXT, GOLD_BORDER);
    _tft.setCursor(14, 6);
    _tft.print("PIN 설정");

    _tft.setTextFont(FONT_MEDIUM);
    _tft.setTextColor(SUCCESS_GREEN, CREAM_BG);
    _tft.setCursor(20, 100);
    _tft.print("저장 완료!");
    delay(700);

    if (_onSuccess) _onSuccess();
}
