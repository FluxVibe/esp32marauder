#include "../../configs.h"
#ifdef HAS_AUDIO

#include "MusicScreen.h"
#include "UIManager.h"
#include <string.h>

extern MusicPlayer music_player_obj;
extern UIManager   ui_manager_obj;

// ----------------------------------------------------------------

MusicScreen::MusicScreen(TFT_eSPI& tft)
    : CLIScreen(tft, "Music"),
      _view(MV_FILES),
      _cursor(0),
      _scrollTop(0),
      _lastState(MS_IDLE),
      _lastProgress(-1)
{}

// ----------------------------------------------------------------
// Navigation
// ----------------------------------------------------------------

void MusicScreen::handleUp() {
    if (_view == MV_FILES) {
        if (_cursor > 0) {
            _cursor--;
            if (_cursor < _scrollTop) _scrollTop = _cursor;
        }
    } else {
        music_player_obj.prev();
    }
}

void MusicScreen::handleDown() {
    if (_view == MV_FILES) {
        int n = music_player_obj.getFileCount();
        if (_cursor < n - 1) {
            _cursor++;
            if (_cursor >= _scrollTop + VISIBLE_ROWS)
                _scrollTop = _cursor - VISIBLE_ROWS + 1;
        }
    } else {
        music_player_obj.next();
    }
}

// ----------------------------------------------------------------
// Actions / back
// ----------------------------------------------------------------

void MusicScreen::onAction() {
    if (_view == MV_FILES) {
        int n = music_player_obj.getFileCount();
        if (n == 0) return;
        music_player_obj.play(_cursor);
        switchToPlaying();
    } else {
        // Pause / Resume toggle
        MusicState s = music_player_obj.getState();
        if (s == MS_PLAYING) music_player_obj.pause();
        else if (s == MS_PAUSED) music_player_obj.resume();
    }
}

void MusicScreen::onBack() {
    if (_view == MV_PLAYING) {
        music_player_obj.stop();
        switchToFiles();
    } else {
        ui_manager_obj.showMenu();
    }
}

// ----------------------------------------------------------------
// Transitions
// ----------------------------------------------------------------

void MusicScreen::switchToPlaying() {
    _view = MV_PLAYING;
    _lastState    = MS_IDLE;
    _lastProgress = -1;
    ui_manager_obj.markDirty();
}

void MusicScreen::switchToFiles() {
    _view = MV_FILES;
    music_player_obj.loadFileList();   // refresh list after stop
    ui_manager_obj.markDirty();
}

// ----------------------------------------------------------------
// Update (live polling — triggers redraw on state change)
// ----------------------------------------------------------------

void MusicScreen::update() {
    music_player_obj.update();  // fill A2DP stream buffer

    if (_view == MV_PLAYING) {
        MusicState cur = music_player_obj.getState();
        int        pct = music_player_obj.getProgress();
        if (cur != _lastState || pct != _lastProgress) {
            _lastState    = cur;
            _lastProgress = pct;
            ui_manager_obj.markDirty();
        }
        // If music player stopped on its own (playlist ended), go back to files
        if (cur == MS_IDLE) {
            switchToFiles();
        }
    }
}

// ----------------------------------------------------------------
// Draw
// ----------------------------------------------------------------

void MusicScreen::draw() {
    _tft.fillScreen(CREAM_BG);
    if (_view == MV_FILES) {
        drawFiles();
    } else {
        drawPlaying();
    }
}

void MusicScreen::drawFiles() {
    drawHeader("Music");

    int n = music_player_obj.getFileCount();
    if (n == 0) {
        _tft.setTextFont(FONT_CLI_BODY);
        _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
        const char* msg = "No files in /music/";
        int tw = _tft.textWidth(msg);
        _tft.setCursor((CLI_W - tw) / 2, CLI_H / 2 - 4);
        _tft.print(msg);
        drawFooter("C:Refresh");
        return;
    }

    int y = CLI_BODY_START;
    for (int i = 0; i < VISIBLE_ROWS && (_scrollTop + i) < n; i++) {
        int   idx      = _scrollTop + i;
        bool  sel      = (idx == _cursor);
        uint16_t bg    = sel ? GOLD_BORDER  : CREAM_BG;
        uint16_t fg    = sel ? GOLD_PRIMARY : DARK_TEXT;

        _tft.fillRect(0, y, CLI_W, ROW_H, bg);
        _tft.setTextFont(FONT_CLI_BODY);
        _tft.setTextColor(fg, bg);

        // Truncate filename to fit
        String name = music_player_obj.getFileName(idx);
        if ((int)_tft.textWidth(name.c_str()) > CLI_W - 8) {
            while (name.length() > 0 &&
                   (int)_tft.textWidth((name + "..").c_str()) > CLI_W - 8)
                name.remove(name.length() - 1);
            name += "..";
        }
        _tft.setCursor(4, y + 3);
        _tft.print(name);

        y += ROW_H;
    }

    drawFooter("U:Up C:Play D:Dn");
}

void MusicScreen::drawPlaying() {
    drawHeader("Now Playing");

    MusicState s = music_player_obj.getState();

    int y = CLI_BODY_START + 2;
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(GOLD_PRIMARY, CREAM_BG);

    // Title (truncated)
    String title = music_player_obj.getCurrentTitle();
    if ((int)_tft.textWidth(title.c_str()) > CLI_W - 8) {
        while (title.length() > 0 &&
               (int)_tft.textWidth((title + "..").c_str()) > CLI_W - 8)
            title.remove(title.length() - 1);
        title += "..";
    }
    int tw = _tft.textWidth(title.c_str());
    _tft.setCursor((CLI_W - tw) / 2, y);
    _tft.print(title);
    y += 14;

    // Status line
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    const char* statusStr;
    if (s == MS_CONNECTING)     statusStr = "Connecting...";
    else if (s == MS_PAUSED)    statusStr = "Paused";
    else if (s == MS_PLAYING) {
        String dev = music_player_obj.getConnectedDevice();
        static char devBuf[20];
        if (dev.length() > 0) {
            strncpy(devBuf, dev.c_str(), sizeof(devBuf) - 1);
            devBuf[sizeof(devBuf) - 1] = '\0';
            statusStr = devBuf;
        } else {
            statusStr = "Playing";
        }
    } else {
        statusStr = "Stopped";
    }
    int sw = _tft.textWidth(statusStr);
    _tft.setCursor((CLI_W - sw) / 2, y);
    _tft.print(statusStr);
    y += 18;

    // Progress bar
    int pct = music_player_obj.getProgress();
    drawProgressBar(y, pct);
    y += 14;

    // Percentage text
    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    int pw = _tft.textWidth(pctBuf);
    _tft.setCursor((CLI_W - pw) / 2, y);
    _tft.print(pctBuf);

    const char* footer = (s == MS_PAUSED) ? "U:Prev C:Resume D:Nxt"
                                          : "U:Prev C:Pause D:Nxt";
    drawFooter(footer);
}

void MusicScreen::drawProgressBar(int y, int pct) {
    // ASCII-style progress bar: [XXXXXXXXXX..........]
    const int BAR_CHARS = 14;
    int filled = (pct * BAR_CHARS) / 100;

    char bar[BAR_CHARS + 3];
    bar[0] = '[';
    for (int i = 0; i < BAR_CHARS; i++)
        bar[i + 1] = (i < filled) ? 'X' : '.';
    bar[BAR_CHARS + 1] = ']';
    bar[BAR_CHARS + 2] = '\0';

    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(GOLD_PRIMARY, CREAM_BG);
    int bw = _tft.textWidth(bar);
    _tft.setCursor((CLI_W - bw) / 2, y);
    _tft.print(bar);
}

#endif // HAS_AUDIO
