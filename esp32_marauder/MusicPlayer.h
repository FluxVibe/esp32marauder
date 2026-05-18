#pragma once
#ifdef HAS_AUDIO

#include "configs.h"
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <BluetoothA2DPSource.h>

#ifdef AUDIO_MP3
  // pschatzmann/AudioTools ^0.9.9 required for MP3 support
  #include <AudioCodecs/CodecMP3Helix.h>
  using namespace audio_tools;
#endif

enum MusicState { MS_IDLE, MS_CONNECTING, MS_PLAYING, MS_PAUSED };

class MusicPlayer {
public:
    static MusicPlayer* s_instance;

    void init();
    void enterMusicMode();
    void exitMusicMode();
    void loadFileList();
    void play(int index);
    void pause();
    void resume();
    void stop();
    void next();
    void prev();
    void update();                    // call from main loop to fill stream buffer

    MusicState getState()            const { return _state; }
    String     getCurrentTitle()     const;
    int        getProgress()         const; // 0-100
    int        getFileCount()        const { return _fileCount; }
    String     getFileName(int i)    const;
    String     getConnectedDevice()  const { return _connectedDevice; }

private:
    static const int MAX_FILES      = 64;
    static const int STREAM_BUF_SZ  = 8192;
    static const int FILL_CHUNK     = 512;

    static int32_t a2dp_cb(Frame* frame, int32_t count);

    BluetoothA2DPSource  _a2dp;
    StreamBufferHandle_t _streamBuf  = nullptr;
    File     _file;
    bool     _isWav     = false;
    MusicState _state   = MS_IDLE;
    int      _fileIndex = 0;
    String   _files[MAX_FILES];
    int      _fileCount = 0;
    uint32_t _totalBytes = 0;
    uint32_t _readBytes  = 0;
    String   _connectedDevice;

#ifdef AUDIO_MP3
    // Bridge: decoder writes decoded PCM into the stream buffer
    class Mp3SinkPrint : public Print {
    public:
        StreamBufferHandle_t* buf = nullptr;
        size_t write(uint8_t c) override { return write(&c, 1); }
        size_t write(const uint8_t* data, size_t len) override {
            if (!buf || !*buf) return len;
            xStreamBufferSend(*buf, data, len, 0);
            return len;
        }
    } _mp3Sink;
    MP3DecoderHelix* _mp3Dec = nullptr;
#endif

    void openFile(int idx);
    void fillBuffer();
    void fillWav();
    void fillMp3();
    void advanceToNext();
};

#endif // HAS_AUDIO
