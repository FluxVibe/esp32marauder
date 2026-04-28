#pragma once

#include "CLIScreen.h"

enum PINMode { PIN_VERIFY, PIN_SET };

// 6-digit numeric PIN entry screen (portrait 135x240).
//
// Button mapping:
//   D_BTN (short press) → handleDigitNext()  — digit 0→1→...→9→0
//   U_BTN (CP2 only)    → handleDigitPrev()  — digit 9→8→...→0→9
//   C_BTN               → handleConfirm()    — lock in current digit, advance
//   C_BTN long press    → onBack()           — cancel (only if onFail != nullptr)
//
// UIManager routes button presses here when this screen is active.
class PINScreen : public CLIScreen {
public:
    explicit PINScreen(TFT_eSPI& tft);

    void setMode(PINMode mode);
    // onSuccess is always called after correct PIN / successful save.
    // onFail is called on cancel; if nullptr, cancel is disabled (lock screen).
    void setCallback(void (*onSuccess)(), void (*onFail)() = nullptr);

    void draw()     override;
    void update()   override;
    void onAction() override;  // C_BTN: confirm current digit
    void onBack()   override;  // cancel → onFail

    // Called directly from UIManager button handlers
    void handleDigitNext();   // D_BTN
    void handleDigitPrev();   // U_BTN
    void handleConfirm();     // C_BTN

    void reset();

private:
    static const int PIN_LEN = 6;

    PINMode _mode;
    int     _digits[PIN_LEN];  // confirmed digits (0-9)
    int     _pos;              // next slot to fill (0..PIN_LEN)
    int     _curDigit;         // digit being dialed (0-9)
    bool    _showError;        // flash red slots on wrong PIN

    void (*_onSuccess)();
    void (*_onFail)();

    void drawSlots();
    void drawDigitSelector();
    void buildEnteredString(char* buf) const;  // buf must be PIN_LEN+1
    void verify();
    void saveNew();
};
