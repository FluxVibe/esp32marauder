#pragma once
#ifdef HAS_AUDIO

#include "CLIScreen.h"
#include "../../MusicPlayer.h"

enum MusicView { MV_FILES, MV_DEVICES, MV_PLAYING };

// Music player screen: BT device picker + file browser + now-playing.
//
// MV_FILES:   U=Up  C=Play/SetBT  D=Down  Back=Menu
//   Row 0 = BT status (C opens MV_DEVICES), rows 1..N = music files
//
// MV_DEVICES: U=Up  C=Connect  D=Down  Back=Files
//   Scans for nearby A2DP sinks; user selects one.
//
// MV_PLAYING: U=Prev  C=Pause/Resume  D=Next  Back=Stop+Files
//   C long-press → Volume/EQ overlay: U/D adjust, C cycle focus, hold C close
class MusicScreen : public CLIScreen {
public:
    explicit MusicScreen(TFT_eSPI& tft);

    void draw()     override;
    void update()   override;
    void onAction() override;
    void onBack()   override;

    void handleUp();
    void handleDown();
    void toggleVolumeOverlay();  // called by UIManager on C long-press

private:
    MusicView _view;

    // MV_FILES state (cursor 0 = BT row, 1..N = files)
    int _cursor;
    int _scrollTop;   // first visible file index (not counting BT row)

    // MV_DEVICES state
    int _devCursor;
    int _devScroll;
    int _lastScanCount;

    // MV_PLAYING state
    MusicState _lastState;
    int        _lastProgress;

    static const int ROW_H        = 14;
    // Total visible rows in body area
    static const int VISIBLE_ROWS = (CLI_FOOTER_Y - CLI_BODY_START) / ROW_H;
    // File rows below the fixed BT row
    static const int FILE_ROWS    = VISIBLE_ROWS - 1;
    // Volume/EQ overlay height (3 rows + border)
    static const int OVL_H        = 49;

    // Volume/EQ overlay (shown over MV_PLAYING)
    bool _showOverlay  = false;
    int  _overlayFocus = 0;   // 0=Volume  1=Bass  2=Treble

    void drawOverlay();
    void drawFiles();
    void drawDevices();
    void drawPlaying();
    void drawProgressBar(int y, int pct);
    void switchToPlaying();
    void switchToFiles();
    void switchToDevices();
};

#endif // HAS_AUDIO
