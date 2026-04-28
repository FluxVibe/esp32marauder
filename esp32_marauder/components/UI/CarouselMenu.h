#pragma once

#include <TFT_eSPI.h>
#include "Theme.h"
#include "Animations.h"

// One item in the carousel.
// action is a zero-argument function pointer called when the card is selected.
struct MenuCard {
    const char* title;
    const char* subtitle;
    void (*action)();
};

// RetroArch / EmulationStation-style carousel.
//
// Horizontal (landscape 240x135, portrait=false):
//   Left card  — offset -CAROUSEL_SIDE_OFFSET, small font, gray
//   Center card — offset 0, large font, gold, subtitle shown
//   Right card  — offset +CAROUSEL_SIDE_OFFSET, small font, gray
//
// Vertical (portrait 135x240, portrait=true):
//   Above card — yOffset -70, small font, gray
//   Center card — yOffset 0, large font, gold, subtitle shown
//   Below card — yOffset +70, small font, gray
//   Indicators drawn on the right edge vertically
//
// On selectNext()/selectPrev() a 300 ms easeInOutCubic slide animation plays.
class CarouselMenu {
public:
    CarouselMenu(MenuCard* items, int count);

    // Renders the full carousel onto tft. Safe to call every loop tick.
    void draw(TFT_eSPI& tft);

    void selectNext();
    void selectPrev();
    void executeSelected();

    bool isAnimating() const;
    int  selectedIndex() const { return _selected; }

    // Switch between horizontal (landscape) and vertical (portrait) layout.
    void setPortraitMode(bool portrait);

private:
    MenuCard*  _cards;
    int        _count;
    int        _selected;
    bool       _portrait;    // false=horizontal, true=vertical

    float     _offset;
    float     _startOffset;
    Animation _anim;

    // Horizontal helpers
    void drawCardH(TFT_eSPI& tft, int index, int xOffset, bool isCenter);
    void drawIndicatorsH(TFT_eSPI& tft);

    // Vertical helpers
    void drawCardV(TFT_eSPI& tft, int index, int yOffset, bool isCenter);
    void drawIndicatorsV(TFT_eSPI& tft);
};
