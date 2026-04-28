#include "CarouselMenu.h"

CarouselMenu::CarouselMenu(MenuCard* items, int count)
    : _cards(items),
      _count(count),
      _selected(0),
      _offset(0.0f),
      _startOffset(0.0f),
      _anim(300)
{}

// ----------------------------------------------------------------
// Public interface
// ----------------------------------------------------------------

bool CarouselMenu::isAnimating() const {
    return _anim.isRunning();
}

void CarouselMenu::selectNext() {
    if (_selected >= _count - 1) return;
    _selected++;
    _startOffset = (float)CAROUSEL_SIDE_OFFSET;
    _offset      = _startOffset;
    _anim.start();
}

void CarouselMenu::selectPrev() {
    if (_selected <= 0) return;
    _selected--;
    _startOffset = -(float)CAROUSEL_SIDE_OFFSET;
    _offset      = _startOffset;
    _anim.start();
}

void CarouselMenu::executeSelected() {
    if (_cards && _cards[_selected].action)
        _cards[_selected].action();
}

// ----------------------------------------------------------------
// Draw
// ----------------------------------------------------------------

void CarouselMenu::draw(TFT_eSPI& tft) {
    // Advance animation
    if (_anim.isRunning()) {
        float t = easeInOutCubic(_anim.getProgress());
        _offset = lerpf(_startOffset, 0.0f, t);
    } else {
        _offset = 0.0f;
    }

    tft.fillScreen(CREAM_BG);

    int xOff = (int)_offset;

    // Left card
    if (_selected > 0)
        drawCard(tft, _selected - 1, xOff - CAROUSEL_SIDE_OFFSET, false);

    // Right card
    if (_selected < _count - 1)
        drawCard(tft, _selected + 1, xOff + CAROUSEL_SIDE_OFFSET, false);

    // Center card drawn last so it renders on top of overlapping text
    drawCard(tft, _selected, xOff, true);

    drawIndicators(tft);
}

// ----------------------------------------------------------------
// Private helpers
// ----------------------------------------------------------------

void CarouselMenu::drawCard(TFT_eSPI& tft, int index,
                            int xOffset, bool isCenter) {
    int cx = CAROUSEL_CX + xOffset;

    // Clip cards that have slid fully off screen
    if (cx < -40 || cx > CAROUSEL_W + 40) return;

    const MenuCard& card = _cards[index];

    if (isCenter) {
        // Large golden title
        tft.setTextFont(FONT_LARGE);
        tft.setTextColor(GOLD_PRIMARY, CREAM_BG);
        int tw = tft.textWidth(card.title);
        tft.setCursor(cx - tw / 2, CAROUSEL_CY - 12);
        tft.print(card.title);

        // Subtitle in small gray
        if (card.subtitle) {
            tft.setTextFont(FONT_SMALL);
            tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
            int sw = tft.textWidth(card.subtitle);
            tft.setCursor(cx - sw / 2, CAROUSEL_CY + 18);
            tft.print(card.subtitle);
        }
    } else {
        // Small gray side title
        tft.setTextFont(FONT_MEDIUM);
        tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
        int tw = tft.textWidth(card.title);
        tft.setCursor(cx - tw / 2, CAROUSEL_CY - 6);
        tft.print(card.title);
    }
}

void CarouselMenu::drawIndicators(TFT_eSPI& tft) {
    if (_count <= 1) return;

    int spacing = 14;
    int totalW  = (_count - 1) * spacing;
    int startX  = CAROUSEL_CX - totalW / 2;

    for (int i = 0; i < _count; i++) {
        int x = startX + i * spacing;
        if (i == _selected) {
            tft.fillCircle(x, CAROUSEL_IND_Y, 4, GOLD_PRIMARY);
        } else {
            tft.drawCircle(x, CAROUSEL_IND_Y, 3, MEDIUM_GRAY);
        }
    }
}
