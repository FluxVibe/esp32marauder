#pragma once
#ifdef HAS_AUDIO

#include "CLIScreen.h"
#include "../../MusicPlayer.h"

enum MusicView { MV_FILES, MV_PLAYING };

// Music player screen: file browser + now-playing view.
//
// MV_FILES:   U=Up  C=Play  D=Down  Back=Menu
// MV_PLAYING: U=Prev  C=Pause/Resume  D=Next  Back=Stop+Menu
class MusicScreen : public CLIScreen {
public:
    explicit MusicScreen(TFT_eSPI& tft);

    void draw()     override;
    void update()   override;
    void onAction() override;
    void onBack()   override;

    void handleUp();
    void handleDown();

private:
    MusicView _view;
    int       _cursor;
    int       _scrollTop;
    MusicState _lastState;
    int        _lastProgress;

    // Rows visible in file browser
    static const int ROW_H        = 14;
    static const int VISIBLE_ROWS = (CLI_FOOTER_Y - CLI_BODY_START) / ROW_H;

    void drawFiles();
    void drawPlaying();
    void drawProgressBar(int y, int pct);
    void switchToPlaying();
    void switchToFiles();
};

#endif // HAS_AUDIO
