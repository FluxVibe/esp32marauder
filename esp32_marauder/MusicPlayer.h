#pragma once
#ifdef HAS_AUDIO

#include "configs.h"
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <BluetoothA2DPSource.h>
#include <Preferences.h>
#include <math.h>

#ifdef AUDIO_MP3
  // pschatzmann/AudioTools ^0.9.9 required for MP3 support
  #include <AudioCodecs/CodecMP3Helix.h>
  using namespace audio_tools;
#endif

enum MusicState { MS_IDLE, MS_CONNECTING, MS_PLAYING, MS_PAUSED };

// RBJ cookbook biquad shelving filter — one instance per channel per band
struct BiquadFilter {
    float b0=1,b1=0,b2=0,a1=0,a2=0;
    float x1=0,x2=0,y1=0,y2=0;

    void setLowShelf(float gainDB, float fc=200.0f, float fs=44100.0f) {
        float A    = powf(10.0f, gainDB / 40.0f);
        float w0   = 2.0f * (float)M_PI * fc / fs;
        float cosw = cosf(w0), sinw = sinf(w0);
        float alpha = sinw * 0.7071f;  // S=1
        float sqA   = sqrtf(A);
        float ap1   = A+1, am1 = A-1;
        float denom = ap1 + am1*cosw + 2*sqA*alpha;
        b0 = A*(ap1 - am1*cosw + 2*sqA*alpha) / denom;
        b1 = 2*A*(am1 - ap1*cosw)             / denom;
        b2 = A*(ap1 - am1*cosw - 2*sqA*alpha) / denom;
        a1 = -2*(am1 + ap1*cosw)              / denom;
        a2 = (ap1 + am1*cosw - 2*sqA*alpha)   / denom;
        x1=x2=y1=y2=0;
    }
    void setHighShelf(float gainDB, float fc=6000.0f, float fs=44100.0f) {
        float A    = powf(10.0f, gainDB / 40.0f);
        float w0   = 2.0f * (float)M_PI * fc / fs;
        float cosw = cosf(w0), sinw = sinf(w0);
        float alpha = sinw * 0.7071f;  // S=1
        float sqA   = sqrtf(A);
        float ap1   = A+1, am1 = A-1;
        float denom = ap1 - am1*cosw + 2*sqA*alpha;
        b0 = A*(ap1 + am1*cosw + 2*sqA*alpha) / denom;
        b1 = -2*A*(am1 + ap1*cosw)            / denom;
        b2 = A*(ap1 + am1*cosw - 2*sqA*alpha) / denom;
        a1 = 2*(am1 - ap1*cosw)               / denom;
        a2 = (ap1 - am1*cosw - 2*sqA*alpha)   / denom;
        x1=x2=y1=y2=0;
    }
    int16_t process(int16_t in) {
        float x = in;
        float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2=x1; x1=x; y2=y1; y1=y;
        if (y >  32767.0f) y =  32767.0f;
        if (y < -32768.0f) y = -32768.0f;
        return (int16_t)y;
    }
};

class MusicPlayer {
public:
    static MusicPlayer* s_instance;

    struct BTDevice {
        char    name[32];
        uint8_t addr[6];
        int     rssi;
    };

    void init();
    void enterMusicMode();   // auto-connect saved device or scan for new
    void exitMusicMode();
    void startDeviceScan();  // explicit scan: show device list in MV_DEVICES
    void connectTo(int index);
    void loadFileList();
    void play(int index);
    void pause();
    void resume();
    void stop();
    void next();
    void prev();
    void update();

    // Volume + EQ
    void setVolume(int pct);
    void setBassGain(int db);
    void setTrebleGain(int db);
    void saveAudioSettings();
    int  getVolume()   const { return _volumePct; }
    int  getBassDB()   const { return _bassDB; }
    int  getTrebleDB() const { return _trebleDB; }

    MusicState getState()            const { return _state; }
    String     getCurrentTitle()     const;
    int        getProgress()         const;
    int        getFileCount()        const { return _fileCount; }
    String     getFileName(int i)    const;
    String     getConnectedDevice()  const { return _connectedDevice; }
    String     getSavedDeviceName()  const { return _savedDeviceName; }
    bool       hasSavedDevice()      const { return _hasSavedDevice; }
    bool       isScanning()          const { return _scanning; }
    int        getDiscoverCount()    const { return _discoverCount; }
    BTDevice   getDiscoveredDevice(int i) const;

private:
    static const int MAX_FILES     = 64;
    static const int MAX_SCAN      = 8;
    static const int STREAM_BUF_SZ = 8192;
    static const int FILL_CHUNK    = 512;

    static int32_t a2dp_cb(Frame* frame, int32_t count);
    static bool    ssid_filter(const char* name, esp_bd_addr_t addr, int rssi);

    void _startBT(bool savedDevice);  // common BT init + start

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

    // Device discovery
    BTDevice _discovered[MAX_SCAN];
    int      _discoverCount = 0;
    bool     _scanning      = false;
    int      _targetIdx     = -1;

    // Persisted device (NVS Preferences)
    String   _savedDeviceName;
    uint8_t  _savedAddr[6] = {};
    bool     _hasSavedDevice = false;

    void _saveDevice(int idx);

    // Volume + EQ state
    int          _volumePct = 30;  // 0-100, default 30% (safe level)
    int          _bassDB    = 0;   // -9..+9 dB, 3dB steps
    int          _trebleDB  = 0;   // -9..+9 dB, 3dB steps
    BiquadFilter _bfBassL, _bfBassR;
    BiquadFilter _bfTrebL, _bfTrebR;

#ifdef AUDIO_MP3
    class Mp3SinkPrint : public Print {
    public:
        StreamBufferHandle_t* buf   = nullptr;
        MusicPlayer*          owner = nullptr;
        size_t write(uint8_t c) override { return write(&c, 1); }
        size_t write(const uint8_t* data, size_t len) override {
            if (!buf || !*buf) return len;
            if (owner && len >= 4 &&
                (owner->_bassDB != 0 || owner->_trebleDB != 0)) {
                uint8_t tmp[len];
                memcpy(tmp, data, len);
                int16_t* s = (int16_t*)tmp;
                size_t frames = len / 4;  // stereo 16-bit = 4 bytes per frame
                for (size_t i = 0; i < frames; i++) {
                    s[i*2]   = owner->_bfTrebL.process(
                                   owner->_bfBassL.process(s[i*2]));
                    s[i*2+1] = owner->_bfTrebR.process(
                                   owner->_bfBassR.process(s[i*2+1]));
                }
                xStreamBufferSend(*buf, tmp, len, 0);
                return len;
            }
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
