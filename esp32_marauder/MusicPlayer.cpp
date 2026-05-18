#include "configs.h"
#ifdef HAS_AUDIO

#include "MusicPlayer.h"
#include "WiFiScan.h"
#include <SD.h>
#include <esp_bt.h>

extern WiFiScan wifi_scan_obj;

MusicPlayer* MusicPlayer::s_instance = nullptr;

// ----------------------------------------------------------------
// Init — load saved device from NVS
// ----------------------------------------------------------------

void MusicPlayer::init() {
    s_instance = this;
    _streamBuf = xStreamBufferCreate(STREAM_BUF_SZ, sizeof(Frame));

    Preferences prefs;
    prefs.begin("music", true);
    _savedDeviceName = prefs.getString("bt_name", "");
    prefs.getBytes("bt_addr", _savedAddr, 6);
    _volumePct  = prefs.getInt("volume", 30);
    _bassDB     = prefs.getInt("bass",   0);
    _trebleDB   = prefs.getInt("treble", 0);
    prefs.end();
    _hasSavedDevice = (_savedDeviceName.length() > 0);

    _bfBassL.setLowShelf(_bassDB);    _bfBassR.setLowShelf(_bassDB);
    _bfTrebL.setHighShelf(_trebleDB); _bfTrebR.setHighShelf(_trebleDB);
}

// ----------------------------------------------------------------
// File list
// ----------------------------------------------------------------

void MusicPlayer::loadFileList() {
    _fileCount = 0;
    File dir = SD.open("/music");
    if (!dir || !dir.isDirectory()) return;

    File f;
    while ((f = dir.openNextFile())) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            String lower = name;
            lower.toLowerCase();
            if (lower.endsWith(".wav") || lower.endsWith(".mp3")) {
                if (_fileCount < MAX_FILES)
                    _files[_fileCount++] = String("/music/") + name;
            }
        }
        f.close();
    }
    dir.close();
}

String MusicPlayer::getFileName(int i) const {
    if (i < 0 || i >= _fileCount) return "";
    int slash = _files[i].lastIndexOf('/');
    return (slash >= 0) ? _files[i].substring(slash + 1) : _files[i];
}

String MusicPlayer::getCurrentTitle() const {
    return getFileName(_fileIndex);
}

int MusicPlayer::getProgress() const {
    if (_totalBytes == 0) return 0;
    return (int)((_readBytes * 100UL) / _totalBytes);
}

MusicPlayer::BTDevice MusicPlayer::getDiscoveredDevice(int i) const {
    if (i >= 0 && i < _discoverCount) return _discovered[i];
    BTDevice empty = {};
    return empty;
}

// ----------------------------------------------------------------
// Internal BT start helper
// ----------------------------------------------------------------

void MusicPlayer::_startBT(bool useSaved) {
    wifi_scan_obj.stopAllScans();
    if (btStarted()) btStop();
    delay(100);

    _a2dp.set_ssid_callback(ssid_filter);
    _a2dp.set_data_callback_in_frames(MusicPlayer::a2dp_cb);

    if (useSaved) {
        // v1.8.6+: start_raw(addr) removed — use set_auto_reconnect(addr) + start()
        _a2dp.set_auto_reconnect(_savedAddr, 3);
        _a2dp.start("Marauder");
    } else {
        _a2dp.set_auto_reconnect(false);
        _a2dp.start("Marauder");
    }
}

// ----------------------------------------------------------------
// BT mode switching
// ----------------------------------------------------------------

void MusicPlayer::enterMusicMode() {
    _discoverCount = 0;
    _scanning      = true;
    _targetIdx     = -1;
    _state         = MS_CONNECTING;

    _startBT(_hasSavedDevice);
}

void MusicPlayer::startDeviceScan() {
    // Hard reset: ensure clean BT state for fresh scan
    if (_state != MS_IDLE) {
        if (_file) _file.close();
        if (_streamBuf) xStreamBufferReset(_streamBuf);
#ifdef AUDIO_MP3
        if (_mp3Dec) { delete _mp3Dec; _mp3Dec = nullptr; }
#endif
        _a2dp.end();
        if (btStarted()) btStop();
        delay(100);
    }

    _discoverCount = 0;
    _scanning      = true;
    _targetIdx     = -1;
    _state         = MS_CONNECTING;
    _connectedDevice = "";

    // Scan without auto-connecting — ssid_filter returns false until user picks
    wifi_scan_obj.stopAllScans();
    if (btStarted()) btStop();
    delay(100);

    _a2dp.set_auto_reconnect(false);
    _a2dp.set_ssid_callback(ssid_filter);
    _a2dp.set_data_callback_in_frames(MusicPlayer::a2dp_cb);
    _a2dp.start("Marauder");
}

void MusicPlayer::connectTo(int index) {
    if (index < 0 || index >= _discoverCount) return;
    _targetIdx = index;

    // Restart targeting selected device by MAC address
    _a2dp.end();
    if (btStarted()) btStop();
    delay(100);

    _scanning = false;  // done scanning, now connecting
    _state    = MS_CONNECTING;

    // v1.8.6+: set_auto_reconnect(addr) replaces start_raw(addr, cb)
    _a2dp.set_auto_reconnect(_discovered[index].addr, 3);
    _a2dp.set_ssid_callback(ssid_filter);
    _a2dp.set_data_callback_in_frames(MusicPlayer::a2dp_cb);
    _a2dp.start("Marauder");
}

void MusicPlayer::exitMusicMode() {
    if (_file) _file.close();
    if (_streamBuf) xStreamBufferReset(_streamBuf);

#ifdef AUDIO_MP3
    if (_mp3Dec) { delete _mp3Dec; _mp3Dec = nullptr; }
#endif

    _a2dp.end();
    _state           = MS_IDLE;
    _scanning        = false;
    _connectedDevice = "";
}

// ----------------------------------------------------------------
// Playback control
// ----------------------------------------------------------------

void MusicPlayer::play(int index) {
    if (index < 0 || index >= _fileCount) return;
    _fileIndex = index;

    if (_state == MS_IDLE) {
        enterMusicMode();  // will open file once connected
    } else if (_state == MS_CONNECTING) {
        // Already connecting — file will be opened in update()
    } else {
        // Already connected — switch track immediately
        if (_file) _file.close();
        xStreamBufferReset(_streamBuf);
        openFile(_fileIndex);
        _state = MS_PLAYING;
    }
}

void MusicPlayer::pause()  { if (_state == MS_PLAYING) _state = MS_PAUSED; }
void MusicPlayer::resume() { if (_state == MS_PAUSED)  _state = MS_PLAYING; }
void MusicPlayer::stop()   { exitMusicMode(); }

void MusicPlayer::next() {
    if (_fileCount == 0) return;
    play((_fileIndex + 1) % _fileCount);
}

void MusicPlayer::prev() {
    if (_fileCount == 0) return;
    play((_fileIndex - 1 + _fileCount) % _fileCount);
}

// ----------------------------------------------------------------
// ssid_filter  (runs in BT task — keep minimal)
// ----------------------------------------------------------------

bool MusicPlayer::ssid_filter(const char* name, esp_bd_addr_t addr, int rssi) {
    MusicPlayer* p = s_instance;
    if (!p) return false;

    // Dedup by MAC address
    for (int i = 0; i < p->_discoverCount; i++) {
        if (memcmp(p->_discovered[i].addr, addr, 6) == 0) {
            if (rssi > p->_discovered[i].rssi) p->_discovered[i].rssi = rssi;
            return (p->_targetIdx == i);  // connect if this is the selected device
        }
    }

    // New device — add to list
    if (p->_discoverCount < MAX_SCAN) {
        BTDevice& d = p->_discovered[p->_discoverCount];
        strncpy(d.name, (name && name[0]) ? name : "(unknown)", 31);
        d.name[31] = '\0';
        memcpy(d.addr, addr, 6);
        d.rssi = rssi;
        int idx = p->_discoverCount++;
        return (p->_targetIdx == idx);
    }
    return false;
}

// ----------------------------------------------------------------
// A2DP callback  (BT task context)
// ----------------------------------------------------------------

int32_t MusicPlayer::a2dp_cb(Frame* frame, int32_t count) {
    MusicPlayer* p = s_instance;
    size_t needed = count * sizeof(Frame);

    if (!p || p->_state != MS_PLAYING) {
        memset(frame, 0, needed);
        return count;
    }

    size_t got = xStreamBufferReceive(p->_streamBuf, frame, needed, 0);
    if (got < needed)
        memset((uint8_t*)frame + got, 0, needed - got);

    return count;
}

// ----------------------------------------------------------------
// update()  — main loop: detect connection, fill stream buffer
// ----------------------------------------------------------------

void MusicPlayer::update() {
    if (_state == MS_CONNECTING && _a2dp.is_connected()) {
        _connectedDevice = String(_a2dp.get_connected_source_name());
        _scanning        = false;

        // Persist the connected device for next session
        if (_targetIdx >= 0 && _targetIdx < _discoverCount) {
            _saveDevice(_targetIdx);
        } else if (_hasSavedDevice) {
            // reconnected to saved device — no need to re-save
        }

        setVolume(_volumePct);  // apply volume once A2DP is active
        openFile(_fileIndex);
        _state = MS_PLAYING;
    }

    if (_state == MS_PLAYING) {
        fillBuffer();
    }
}

// ----------------------------------------------------------------
// Volume + EQ control
// ----------------------------------------------------------------

void MusicPlayer::setVolume(int pct) {
    _volumePct = constrain(pct, 0, 100);
    _a2dp.set_volume(_volumePct * 127 / 100);
}

void MusicPlayer::setBassGain(int db) {
    _bassDB = constrain(db, -9, 9);
    _bfBassL.setLowShelf(_bassDB);
    _bfBassR.setLowShelf(_bassDB);
}

void MusicPlayer::setTrebleGain(int db) {
    _trebleDB = constrain(db, -9, 9);
    _bfTrebL.setHighShelf(_trebleDB);
    _bfTrebR.setHighShelf(_trebleDB);
}

void MusicPlayer::saveAudioSettings() {
    Preferences prefs;
    prefs.begin("music", false);
    prefs.putInt("volume",  _volumePct);
    prefs.putInt("bass",    _bassDB);
    prefs.putInt("treble",  _trebleDB);
    prefs.end();
}

void MusicPlayer::_saveDevice(int idx) {
    Preferences prefs;
    prefs.begin("music", false);
    prefs.putString("bt_name", _discovered[idx].name);
    prefs.putBytes ("bt_addr", _discovered[idx].addr, 6);
    prefs.end();
    _savedDeviceName = String(_discovered[idx].name);
    memcpy(_savedAddr, _discovered[idx].addr, 6);
    _hasSavedDevice  = true;
}

// ----------------------------------------------------------------
// File open + fill helpers
// ----------------------------------------------------------------

void MusicPlayer::openFile(int idx) {
    if (_file) _file.close();
    xStreamBufferReset(_streamBuf);
    _readBytes  = 0;
    _totalBytes = 0;

    if (idx < 0 || idx >= _fileCount) return;

    _file = SD.open(_files[idx]);
    if (!_file) return;

    _totalBytes = _file.size();
    String lower = _files[idx];
    lower.toLowerCase();
    _isWav = lower.endsWith(".wav");

    if (_isWav) {
        if (_totalBytes > 44) {
            _file.seek(44);
            _readBytes  = 44;
            _totalBytes -= 44;
        }
    } else {
#ifdef AUDIO_MP3
        if (_mp3Dec) delete _mp3Dec;
        _mp3Sink.buf   = &_streamBuf;
        _mp3Sink.owner = this;
        _mp3Dec = new MP3DecoderHelix(_mp3Sink);
        _mp3Dec->begin();
#endif
    }
}

void MusicPlayer::fillBuffer() {
    if (xStreamBufferSpacesAvailable(_streamBuf) < FILL_CHUNK) return;
    if (_isWav) fillWav();
    else        fillMp3();
}

void MusicPlayer::fillWav() {
    uint8_t buf[FILL_CHUNK];
    size_t space  = xStreamBufferSpacesAvailable(_streamBuf);
    size_t toRead = min((size_t)FILL_CHUNK, space);
    if (toRead == 0) return;

    size_t n = _file.read(buf, toRead);
    if (n > 0) {
        _readBytes += n;
        if (_bassDB != 0 || _trebleDB != 0) {
            int16_t* s = (int16_t*)buf;
            size_t frames = n / 4;  // stereo 16-bit = 4 bytes/frame
            for (size_t i = 0; i < frames; i++) {
                s[i*2]   = _bfTrebL.process(_bfBassL.process(s[i*2]));
                s[i*2+1] = _bfTrebR.process(_bfBassR.process(s[i*2+1]));
            }
        }
        xStreamBufferSend(_streamBuf, buf, n, 0);
    }
    if (n == 0 || !_file.available()) advanceToNext();
}

void MusicPlayer::fillMp3() {
#ifdef AUDIO_MP3
    if (!_mp3Dec) return;
    uint8_t buf[FILL_CHUNK];
    size_t n = _file.read(buf, FILL_CHUNK);
    if (n > 0) {
        _readBytes += n;
        _mp3Dec->write(buf, n);
    }
    if (n == 0 || !_file.available()) advanceToNext();
#else
    advanceToNext();
#endif
}

void MusicPlayer::advanceToNext() {
    if (_fileCount == 0) { stop(); return; }
    int nxt = (_fileIndex + 1) % _fileCount;
    if (nxt == 0 && _fileCount > 1) stop();  // end of playlist
    else play(nxt);
}

#endif // HAS_AUDIO
