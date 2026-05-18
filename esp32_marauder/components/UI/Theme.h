#pragma once

// =============================================================
//  solinlab color palette — RGB565 encoded
// =============================================================

// Background & base
#define CREAM_BG       0xF7BE  // #F5F1E8  cream background
#define DARK_TEXT      0x2104  // #2C2C2C  main text
#define MEDIUM_GRAY    0x8410  // #888888  inactive / sub text

// Accent & highlight
#define GOLD_PRIMARY   0xFEA0  // #D4A574  selected menu card
#define GOLD_BORDER    0xE73C  // #E5DCC8  header bar / borders
#define WARM_ACCENT    0xFD20  // #E8B888  accent

// Status colors
#define SUCCESS_GREEN  0x07E0  // pure green
#define WARNING_ORANGE 0xFC00  // orange
#define ERROR_RED      0xF800  // red

// =============================================================
//  Font size aliases  (TFT_eSPI built-in font numbers)
// =============================================================

// Carousel / landscape mode
#define FONT_LARGE     4   // ~26pt — center card title
#define FONT_MEDIUM    2   // ~16pt — side card titles
#define FONT_SMALL     1   //  ~8pt — subtitles

// CLI / portrait mode
#define FONT_CLI_BODY  1   //  ~8pt — log lines
#define FONT_CLI_HEAD  2   // ~16pt — header bar

// =============================================================
//  Layout constants — display-specific
// =============================================================

#ifdef MARAUDER_MINI
  // 128×128 square ST7735 (portrait-only mode)
  #define CAROUSEL_W            128
  #define CAROUSEL_H            128
  #define CAROUSEL_CX            64
  #define CAROUSEL_CY            50
  #define CAROUSEL_SIDE_OFFSET   65
  #define CAROUSEL_IND_Y        120

  #define CLI_W          128
  #define CLI_H          128
  #define CLI_HEADER_H    16
  #define CLI_FOOTER_Y   112
  #define CLI_BODY_START  18
#else
  // M5StickC / default: landscape carousel 240×135, portrait CLI 135×240
  #define CAROUSEL_W            240
  #define CAROUSEL_H            135
  #define CAROUSEL_CX           120
  #define CAROUSEL_CY            55
  #define CAROUSEL_SIDE_OFFSET  110
  #define CAROUSEL_IND_Y        122

  #define CLI_W          135
  #define CLI_H          240
  #define CLI_HEADER_H    20
  #define CLI_FOOTER_Y   225
  #define CLI_BODY_START  22
#endif
