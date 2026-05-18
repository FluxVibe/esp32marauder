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
      _cursor(0), _scrollTop(0),
      _devCursor(0), _devScroll(0), _lastScanCount(-1),
      _lastState(MS_IDLE), _lastProgress(-1)
{}

// ----------------------------------------------------------------
// Navigation
// ----------------------------------------------------------------

void MusicScreen::handleUp() {
    if (_view == MV_FILES) {
        if (_cursor > 0) {
            _cursor--;
            // Adjust scroll for file rows (cursor>0 means a file is selected)
            if (_cursor > 0) {
                int fileRow = _cursor - 1;
                if (fileRow < _scrollTop) _scrollTop = fileRow;
            }
        }
    } else if (_view == MV_DEVICES) {
        if (_devCursor > 0) {
            _devCursor--;
            if (_devCursor < _devScroll) _devScroll = _devCursor;
        }
    } else {
        music_player_obj.prev();
    }
}

void MusicScreen::handleDown() {
    if (_view == MV_FILES) {
        int n = music_player_obj.getFileCount();
        if (_cursor <= n) {  // 0=BT, 1..n=files
            _cursor++;
            if (_cursor > 0) {
                int fileRow = _cursor - 1;
                if (fileRow >= _scrollTop + FILE_ROWS)
                    _scrollTop = fileRow - FILE_ROWS + 1;
            }
        }
    } else if (_view == MV_DEVICES) {
        int n = music_player_obj.getDiscoverCount();
        if (_devCursor < n - 1) {
            _devCursor++;
            if (_devCursor >= _devScroll + VISIBLE_ROWS)
                _devScroll = _devCursor - VISIBLE_ROWS + 1;
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
        if (_cursor == 0) {
            switchToDevices();
        } else {
            int fileIdx = _cursor - 1;
            if (fileIdx < music_player_obj.getFileCount()) {
                music_player_obj.play(fileIdx);
                switchToPlaying();
            }
        }
    } else if (_view == MV_DEVICES) {
        if (music_player_obj.getDiscoverCount() > 0 &&
            _devCursor < music_player_obj.getDiscoverCount()) {
            music_player_obj.connectTo(_devCursor);
            // Go to playing view — will show "Connecting..."
            _view = MV_PLAYING;
            _lastState = MS_IDLE;
            _lastProgress = -1;
            ui_manager_obj.markDirty();
        }
    } else {
        MusicState s = music_player_obj.getState();
        if      (s == MS_PLAYING) music_player_obj.pause();
        else if (s == MS_PAUSED)  music_player_obj.resume();
    }
}

void MusicScreen::onBack() {
    if (_view == MV_DEVICES) {
        _view = MV_FILES;
        ui_manager_obj.markDirty();
    } else if (_view == MV_PLAYING) {
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
    music_player_obj.loadFileList();
    ui_manager_obj.markDirty();
}

void MusicScreen::switchToDevices() {
    _devCursor = 0;
    _devScroll = 0;
    _lastScanCount = -1;
    music_player_obj.startDeviceScan();
    _view = MV_DEVICES;
    ui_manager_obj.markDirty();
}

// ----------------------------------------------------------------
// Update
// ----------------------------------------------------------------

void MusicScreen::update() {
    music_player_obj.update();

    if (_view == MV_DEVICES) {
        int cur = music_player_obj.getDiscoverCount();
        bool scanning = music_player_obj.isScanning();
        if (cur != _lastScanCount) {
            _lastScanCount = cur;
            ui_manager_obj.markDirty();
        }
        // Scan ended with no connection → stay in device list
        if (!scanning && music_player_obj.getState() == MS_IDLE) {
            ui_manager_obj.markDirty();
        }
    } else if (_view == MV_PLAYING) {
        MusicState cur = music_player_obj.getState();
        int        pct = music_player_obj.getProgress();
        if (cur != _lastState || pct != _lastProgress) {
            _lastState    = cur;
            _lastProgress = pct;
            ui_manager_obj.markDirty();
        }
        if (cur == MS_IDLE) switchToFiles();  // playlist ended
    }
}

// ----------------------------------------------------------------
// Draw dispatch
// ----------------------------------------------------------------

void MusicScreen::draw() {
    _tft.fillScreen(CREAM_BG);
    if      (_view == MV_FILES)   drawFiles();
    else if (_view == MV_DEVICES) drawDevices();
    else                          drawPlaying();
}

// ----------------------------------------------------------------
// MV_FILES — BT status row + file list
// ----------------------------------------------------------------

void MusicScreen::drawFiles() {
    drawHeader("Music");

    // ---- Row 0: BT status (always at top) ----
    bool     btSel = (_cursor == 0);
    uint16_t btBg  = btSel ? GOLD_BORDER : CREAM_BG;
    uint16_t btFg  = btSel ? GOLD_PRIMARY : MEDIUM_GRAY;

    _tft.fillRect(0, CLI_BODY_START, CLI_W, ROW_H, btBg);
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(btFg, btBg);

    MusicState st = music_player_obj.getState();
    String btLabel;
    if (st == MS_PLAYING || st == MS_PAUSED || st == MS_CONNECTING) {
        String dev = music_player_obj.getConnectedDevice();
        btLabel = dev.length() > 0 ? "BT: " + dev : "BT: Connecting...";
    } else if (music_player_obj.hasSavedDevice()) {
        btLabel = "BT: " + music_player_obj.getSavedDeviceName();
    } else {
        btLabel = "BT: Not Connected";
    }
    // Truncate to fit
    while (btLabel.length() > 0 &&
           (int)_tft.textWidth(btLabel.c_str()) > CLI_W - 8)
        btLabel.remove(btLabel.length() - 1);
    _tft.setCursor(4, CLI_BODY_START + 3);
    _tft.print(btLabel);

    // ---- File rows ----
    int n = music_player_obj.getFileCount();
    int y = CLI_BODY_START + ROW_H;

    if (n == 0) {
        _tft.setTextFont(FONT_CLI_BODY);
        _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
        _tft.setCursor(4, y + 3);
        _tft.print("No files in /music/");
    } else {
        for (int i = 0; i < FILE_ROWS && (_scrollTop + i) < n; i++) {
            int      fileIdx = _scrollTop + i;
            bool     sel     = (_cursor > 0 && fileIdx == (_cursor - 1));
            uint16_t bg      = sel ? GOLD_BORDER : CREAM_BG;
            uint16_t fg      = sel ? GOLD_PRIMARY : DARK_TEXT;

            _tft.fillRect(0, y, CLI_W, ROW_H, bg);
            _tft.setTextFont(FONT_CLI_BODY);
            _tft.setTextColor(fg, bg);

            String name = music_player_obj.getFileName(fileIdx);
            while (name.length() > 0 &&
                   (int)_tft.textWidth(name.c_str()) > CLI_W - 8)
                name.remove(name.length() - 1);
            _tft.setCursor(4, y + 3);
            _tft.print(name);
            y += ROW_H;
        }
    }

    const char* footer = btSel ? "C:Set BT D:Files" : "U:Up C:Play D:Dn";
    drawFooter(footer);
}

// ----------------------------------------------------------------
// MV_DEVICES — nearby A2DP sink scanner
// ----------------------------------------------------------------

void MusicScreen::drawDevices() {
    bool scanning = music_player_obj.isScanning();
    drawHeader(scanning ? "Scanning..." : "BT Devices");

    int n = music_player_obj.getDiscoverCount();

    if (n == 0) {
        _tft.setTextFont(FONT_CLI_BODY);
        _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
        const char* msg = scanning ? "Scanning..." : "No devices found";
        int mw = _tft.textWidth(msg);
        _tft.setCursor((CLI_W - mw) / 2, CLI_H / 2 - 4);
        _tft.print(msg);
    } else {
        int y = CLI_BODY_START;
        for (int i = _devScroll;
             i < n && (i - _devScroll) < VISIBLE_ROWS;
             i++) {
            bool sel = (i == _devCursor);
            MusicPlayer::BTDevice dev = music_player_obj.getDiscoveredDevice(i);

            uint16_t bg = sel ? GOLD_BORDER : CREAM_BG;
            uint16_t fg = sel ? GOLD_PRIMARY : DARK_TEXT;

            _tft.fillRect(0, y, CLI_W, ROW_H, bg);
            _tft.setTextFont(FONT_CLI_BODY);
            _tft.setTextColor(fg, bg);

            // Truncate device name to fit display width
            String name = String(dev.name);
            while (name.length() > 0 &&
                   (int)_tft.textWidth(name.c_str()) > CLI_W - 8)
                name.remove(name.length() - 1);
            _tft.setCursor(4, y + 3);
            _tft.print(name);
            y += ROW_H;
        }
    }

    drawFooter(n > 0 ? "U:Up C:Conn D:Dn" : "Back:Cancel");
}

// ----------------------------------------------------------------
// MV_PLAYING — now-playing view
// ----------------------------------------------------------------

void MusicScreen::drawPlaying() {
    drawHeader("Now Playing");

    MusicState s = music_player_obj.getState();
    int y = CLI_BODY_START + 2;

    // Title
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(GOLD_PRIMARY, CREAM_BG);
    String title = music_player_obj.getCurrentTitle();
    while (title.length() > 0 &&
           (int)_tft.textWidth(title.c_str()) > CLI_W - 8)
        title.remove(title.length() - 1);
    int tw = _tft.textWidth(title.c_str());
    _tft.setCursor((CLI_W - tw) / 2, y);
    _tft.print(title);
    y += 14;

    // Status
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    static char statusBuf[24];
    if (s == MS_CONNECTING) {
        strncpy(statusBuf, "Connecting...", sizeof(statusBuf));
    } else if (s == MS_PAUSED) {
        strncpy(statusBuf, "Paused", sizeof(statusBuf));
    } else if (s == MS_PLAYING) {
        String dev = music_player_obj.getConnectedDevice();
        strncpy(statusBuf, dev.length() > 0 ? dev.c_str() : "Playing",
                sizeof(statusBuf) - 1);
        statusBuf[sizeof(statusBuf) - 1] = '\0';
    } else {
        strncpy(statusBuf, "Stopped", sizeof(statusBuf));
    }
    // Truncate status to fit
    String statusStr = String(statusBuf);
    while (statusStr.length() > 0 &&
           (int)_tft.textWidth(statusStr.c_str()) > CLI_W - 8)
        statusStr.remove(statusStr.length() - 1);
    int sw = _tft.textWidth(statusStr.c_str());
    _tft.setCursor((CLI_W - sw) / 2, y);
    _tft.print(statusStr);
    y += 18;

    // Progress bar
    int pct = music_player_obj.getProgress();
    drawProgressBar(y, pct);
    y += 14;

    // Percentage
    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(MEDIUM_GRAY, CREAM_BG);
    int pw = _tft.textWidth(pctBuf);
    _tft.setCursor((CLI_W - pw) / 2, y);
    _tft.print(pctBuf);

    drawFooter((s == MS_PAUSED) ? "U:Prev C:Resume D:Nxt"
                                : "U:Prev C:Pause D:Nxt");
}

void MusicScreen::drawProgressBar(int y, int pct) {
    const int BAR_CHARS = 14;
    int filled = (pct * BAR_CHARS) / 100;
    char bar[BAR_CHARS + 3];
    bar[0] = '[';
    for (int i = 0; i < BAR_CHARS; i++) bar[i + 1] = (i < filled) ? 'X' : '.';
    bar[BAR_CHARS + 1] = ']';
    bar[BAR_CHARS + 2] = '\0';

    _tft.setTextFont(FONT_CLI_BODY);
    _tft.setTextColor(GOLD_PRIMARY, CREAM_BG);
    int bw = _tft.textWidth(bar);
    _tft.setCursor((CLI_W - bw) / 2, y);
    _tft.print(bar);
}

#endif // HAS_AUDIO
