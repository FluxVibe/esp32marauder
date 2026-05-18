#include "configs.h"
#ifdef HAS_AUDIO

#include "MusicPlayer.h"
#include "WiFiScan.h"
#include <SD.h>
#include <esp_bt.h>

extern WiFiScan wifi_scan_obj;

MusicPlayer* MusicPlayer::s_instance = nullptr;

// ----------------------------------------------------------------
// Init
// ----------------------------------------------------------------

void MusicPlayer::init() {
    s_instance  = this;
    _streamBuf  = xStreamBufferCreate(STREAM_BUF_SZ, sizeof(Frame));
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
                if (_fileCount < MAX_FILES) {
                    // f.name() on SD lib returns short name only; prepend path
                    _files[_fileCount++] = String("/music/") + name;
                }
            }
        }
        f.close();
    }
    dir.close();
}

String MusicPlayer::getFileName(int i) const {
    if (i < 0 || i >= _fileCount) return "";
    // Return basename only (after last '/')
    int slash = _files[i].lastIndexOf('/');
    if (slash >= 0) return _files[i].substring(slash + 1);
    return _files[i];
}

String MusicPlayer::getCurrentTitle() const {
    return getFileName(_fileIndex);
}

int MusicPlayer::getProgress() const {
    if (_totalBytes == 0) return 0;
    return (int)((_readBytes * 100UL) / _totalBytes);
}

// ----------------------------------------------------------------
// BT mode switching
// ----------------------------------------------------------------

void MusicPlayer::enterMusicMode() {
    wifi_scan_obj.stopAllScans();  // stop WiFi + deinit NimBLE

    // Ensure BT controller is down before A2DP reinitialises it in classic mode
    if (btStarted()) btStop();
    delay(50);

    _a2dp.set_auto_reconnect(false);
    _a2dp.start("Marauder", MusicPlayer::a2dp_cb);
    _state = MS_CONNECTING;
}

void MusicPlayer::exitMusicMode() {
    if (_file) _file.close();
    if (_streamBuf) xStreamBufferReset(_streamBuf);

#ifdef AUDIO_MP3
    if (_mp3Dec) {
        delete _mp3Dec;
        _mp3Dec = nullptr;
    }
#endif

    _a2dp.end();
    _state = MS_IDLE;
    _connectedDevice = "";
}

// ----------------------------------------------------------------
// Playback control
// ----------------------------------------------------------------

void MusicPlayer::play(int index) {
    if (index < 0 || index >= _fileCount) return;
    _fileIndex = index;

    if (_state == MS_IDLE || _state == MS_CONNECTING) {
        enterMusicMode();  // starts A2DP; playback begins when connected
    } else {
        // Already in music mode — just open the new file
        if (_file) _file.close();
        xStreamBufferReset(_streamBuf);
        openFile(_fileIndex);
        _state = MS_PLAYING;
    }
}

void MusicPlayer::pause() {
    if (_state == MS_PLAYING) _state = MS_PAUSED;
}

void MusicPlayer::resume() {
    if (_state == MS_PAUSED) _state = MS_PLAYING;
}

void MusicPlayer::stop() {
    exitMusicMode();
}

void MusicPlayer::next() {
    if (_fileCount == 0) return;
    int next = (_fileIndex + 1) % _fileCount;
    play(next);
}

void MusicPlayer::prev() {
    if (_fileCount == 0) return;
    int prev = (_fileIndex - 1 + _fileCount) % _fileCount;
    play(prev);
}

// ----------------------------------------------------------------
// A2DP callback  (runs in BT task)
// ----------------------------------------------------------------

int32_t MusicPlayer::a2dp_cb(Frame* frame, int32_t count) {
    MusicPlayer* p = s_instance;
    if (!p || p->_state == MS_IDLE || p->_state == MS_CONNECTING) {
        // Silence while not yet playing
        memset(frame, 0, count * sizeof(Frame));
        return count;
    }

    if (p->_state == MS_PAUSED) {
        memset(frame, 0, count * sizeof(Frame));
        return count;
    }

    // MS_PLAYING: drain stream buffer
    size_t needed = count * sizeof(Frame);
    size_t got    = xStreamBufferReceive(p->_streamBuf, frame, needed, 0);

    if (got < needed) {
        memset((uint8_t*)frame + got, 0, needed - got);
    }

    // Transition: once connected (first non-zero fill), open file
    if (p->_state == MS_CONNECTING && got > 0) {
        p->_state = MS_PLAYING;
    }

    return count;
}

// ----------------------------------------------------------------
// update()  — called from main loop, feeds stream buffer from SD
// ----------------------------------------------------------------

void MusicPlayer::update() {
    // Once A2DP is connected, open the file for playback
    if (_state == MS_CONNECTING && _a2dp.is_connected()) {
        _connectedDevice = String(_a2dp.get_connected_source_name());
        openFile(_fileIndex);
        _state = MS_PLAYING;
    }

    if (_state == MS_PLAYING) {
        fillBuffer();
    }
}

void MusicPlayer::openFile(int idx) {
    if (_file) _file.close();
    xStreamBufferReset(_streamBuf);
    _readBytes   = 0;
    _totalBytes  = 0;

    if (idx < 0 || idx >= _fileCount) return;

    _file = SD.open(_files[idx]);
    if (!_file) return;

    _totalBytes = _file.size();
    String lower = _files[idx];
    lower.toLowerCase();
    _isWav = lower.endsWith(".wav");

    if (_isWav) {
        // Skip 44-byte PCM WAV header
        if (_totalBytes > 44) {
            _file.seek(44);
            _readBytes  = 44;
            _totalBytes -= 44;  // track audio bytes only
        }
    } else {
#ifdef AUDIO_MP3
        // Re-create decoder so state is clean for a new file
        if (_mp3Dec) delete _mp3Dec;
        _mp3Sink.buf = &_streamBuf;
        _mp3Dec = new MP3DecoderHelix(_mp3Sink);
        _mp3Dec->begin();
#endif
    }
}

void MusicPlayer::fillBuffer() {
    // Don't overfill — leave room for at least one chunk
    if (xStreamBufferSpacesAvailable(_streamBuf) < FILL_CHUNK) return;

    if (_isWav) {
        fillWav();
    } else {
        fillMp3();
    }
}

void MusicPlayer::fillWav() {
    uint8_t buf[FILL_CHUNK];
    size_t space = xStreamBufferSpacesAvailable(_streamBuf);
    size_t toRead = min((size_t)FILL_CHUNK, space);
    if (toRead == 0) return;

    size_t n = _file.read(buf, toRead);
    if (n > 0) {
        _readBytes += n;
        xStreamBufferSend(_streamBuf, buf, n, 0);
    }

    if (n == 0 || !_file.available()) {
        advanceToNext();
    }
}

void MusicPlayer::fillMp3() {
#ifdef AUDIO_MP3
    if (!_mp3Dec) return;
    uint8_t buf[FILL_CHUNK];
    size_t n = _file.read(buf, FILL_CHUNK);
    if (n > 0) {
        _readBytes += n;
        _mp3Dec->write(buf, n);  // decoded PCM flows via _mp3Sink -> _streamBuf
    }
    if (n == 0 || !_file.available()) {
        advanceToNext();
    }
#else
    // No MP3 decoder — skip to next file
    advanceToNext();
#endif
}

void MusicPlayer::advanceToNext() {
    if (_fileCount == 0) {
        stop();
        return;
    }
    int next = (_fileIndex + 1) % _fileCount;
    if (next == 0 && _fileCount > 1) {
        // Wrapped around: stop after last track (single-pass playlist)
        stop();
    } else {
        play(next);
    }
}

#endif // HAS_AUDIO
