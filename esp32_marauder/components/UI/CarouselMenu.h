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

// RetroArch / EmulationStation-style horizontal carousel.
//
// Layout (landscape 240x135):
//   Left card  — offset -CAROUSEL_SIDE_OFFSET, small font, gray
//   Center card — offset 0, large font, gold, subtitle shown
//   Right card  — offset +CAROUSEL_SIDE_OFFSET, small font, gray
//
// On selectNext()/selectPrev() a 300 ms easeInOutCubic slide animation plays.
// Call draw() every loop iteration; it handles animation progress internally.
class CarouselMenu {
public:
    CarouselMenu(MenuCard* items, int count);

    // Renders the full carousel onto tft. Safe to call every loop tick.
    void draw(TFT_eSPI& tft);

    void selectNext();  // slide left (next item comes from right)
    void selectPrev();  // slide right (prev item comes from left)
    void executeSelected();  // invoke the current card's action()

    bool isAnimating() const;
    int  selectedIndex() const { return _selected; }

private:
    MenuCard*  _cards;
    int        _count;
    int        _selected;

    float     _offset;       // current rendered pixel offset (animated toward 0)
    float     _startOffset;  // offset at animation start
    Animation _anim;

    void drawCard(TFT_eSPI& tft, int index, int xOffset, bool isCenter);
    void drawIndicators(TFT_eSPI& tft);
};
