#include "CarouselMenu.h"

// Portrait layout constants — display-specific
#ifdef MARAUDER_MINI
  // 128×128 square
  #define PORT_CX      64
  #define PORT_CY      54
  #define PORT_VOFF    40
  #define PORT_IND_X  122
  #define PORT_IND_START_Y  36
#else
  // 135×240
  #define PORT_CX      67
  #define PORT_CY     100
  #define PORT_VOFF    70
  #define PORT_IND_X  128
  #define PORT_IND_START_Y  80
#endif

CarouselMenu::CarouselMenu(MenuCard* items, int count)
    : _cards(items),
      _count(count),
      _selected(0),
      _portrait(false),
      _offset(0.0f),
      _startOffset(0.0f),
      _anim(300)
{}

// ----------------------------------------------------------------
// Public interface
// ----------------------------------------------------------------

void CarouselMenu::setPortraitMode(bool portrait) {
    _portrait = portrait;
}

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
// Draw — dispatch to horizontal or vertical layout
// ----------------------------------------------------------------

void CarouselMenu::draw(TFT_eSPI& tft) {
    if (_anim.isRunning()) {
        float t = easeInOutCubic(_anim.getProgress());
        _offset = lerpf(_startOffset, 0.0f, t);
    } else {
        _offset = 0.0f;
    }

    tft.fillScreen(CREAM_BG);
    int off = (int)_offset;

    if (_portrait) {
        // Vertical layout (135x240)
        if (_selected > 0)
            drawCardV(tft, _selected - 1, off - PORT_VOFF, false);
        if (_selected < _count - 1)
            drawCardV(tft, _selected + 1, off + PORT_VOFF, false);
        drawCardV(tft, _selected, off, true);
        drawIndicatorsV(tft);
    } else {
        // Horizontal layout (240x135)
        if (_selected > 0)
            drawCardH(tft, _selected - 1, off - CAROUSEL_SIDE_OFFSET, false);
        if (_selected < _count - 1)
            drawCardH(tft, _selected + 1, off + CAROUSEL_SIDE_OFFSET, false);
        drawCardH(tft, _selected, off, true);
        drawIndicatorsH(tft);
    }
}

// ----------------------------------------------------------------
// Horizontal helpers (landscape 240x135)
// ----------------------------------------------------------------

void CarouselMenu::drawCardH(TFT_eSPI& tft, int index,
                             int xOffset, bool isCenter) {
    int cx = CAROUSEL_CX + xOffset;
    if (cx < -40 || cx > CAROUSEL_W + 40) return;

    const MenuCard& card = _cards[index];

    if (isCenter) {
        tft.setTextFont(FONT_LARGE);
        tft.setTextColor(GOLD_PRIMARY, CREAM_BG);
        int tw = tft.textWidth(card.title);
        tft.setCursor(cx - tw / 2, CAROUSEL_CY - 12);
        tft.print(card.title);

        if (card.subtitle) {
            tft.setTextFont(FONT_SMALL);
            tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
            int sw = tft.textWidth(card.subtitle);
            tft.setCursor(cx - sw / 2, CAROUSEL_CY + 18);
            tft.print(card.subtitle);
        }
    } else {
        tft.setTextFont(FONT_MEDIUM);
        tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
        int tw = tft.textWidth(card.title);
        tft.setCursor(cx - tw / 2, CAROUSEL_CY - 6);
        tft.print(card.title);
    }
}

void CarouselMenu::drawIndicatorsH(TFT_eSPI& tft) {
    if (_count <= 1) return;

    int spacing = 14;
    int totalW  = (_count - 1) * spacing;
    int startX  = CAROUSEL_CX - totalW / 2;

    for (int i = 0; i < _count; i++) {
        int x = startX + i * spacing;
        if (i == _selected)
            tft.fillCircle(x, CAROUSEL_IND_Y, 4, GOLD_PRIMARY);
        else
            tft.drawCircle(x, CAROUSEL_IND_Y, 3, MEDIUM_GRAY);
    }
}

// ----------------------------------------------------------------
// Vertical helpers (portrait 135x240)
// ----------------------------------------------------------------

void CarouselMenu::drawCardV(TFT_eSPI& tft, int index,
                             int yOffset, bool isCenter) {
    int cy = PORT_CY + yOffset;
    if (cy < -30 || cy > CLI_H + 30) return;  // off-screen

    const MenuCard& card = _cards[index];

    if (isCenter) {
        tft.setTextFont(FONT_LARGE);
        tft.setTextColor(GOLD_PRIMARY, CREAM_BG);
        int tw = tft.textWidth(card.title);
        tft.setCursor(PORT_CX - tw / 2, cy - 14);
        tft.print(card.title);

        if (card.subtitle) {
            tft.setTextFont(FONT_SMALL);
            tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
            int sw = tft.textWidth(card.subtitle);
            tft.setCursor(PORT_CX - sw / 2, cy + 18);
            tft.print(card.subtitle);
        }
    } else {
        tft.setTextFont(FONT_MEDIUM);
        tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
        int tw = tft.textWidth(card.title);
        tft.setCursor(PORT_CX - tw / 2, cy - 6);
        tft.print(card.title);
    }
}

void CarouselMenu::drawIndicatorsV(TFT_eSPI& tft) {
    if (_count <= 1) return;

    int spacing = 12;
    int startY  = PORT_IND_START_Y;

    for (int i = 0; i < _count; i++) {
        int y = startY + i * spacing;
        if (i == _selected)
            tft.fillCircle(PORT_IND_X, y, 4, GOLD_PRIMARY);
        else
            tft.drawCircle(PORT_IND_X, y, 3, MEDIUM_GRAY);
    }
}
