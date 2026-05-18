#pragma once

#ifndef Buffer_h
#define Buffer_h

#include "Arduino.h"
#include "FS.h"
#include "settings.h"
#include "esp_wifi_types.h"
#include "configs.h"
#include "mbedtls/aes.h"

//#define BUF_SIZE 3 * 1024 // Had to reduce buffer size to save RAM. GG @spacehuhn
//#define SNAP_LEN 2324 // max len of each recieved packet

//extern bool useSD;

extern Settings settings_obj;

class Buffer {
  public:
    Buffer();
    void pcapOpen(String file_name, fs::FS* fs, bool serial);
    void logOpen(String file_name, fs::FS* fs, bool serial);
    void gpxOpen(String file_name, fs::FS* fs, bool serial);
    void append(wifi_promiscuous_pkt_t *packet, int len);
    void append(String log);
    void save();
    String getFileName();
    // AES-128-CTR PCAP encryption.
    // Call enableEncryption() before pcapOpen(); call disableEncryption() when done.
    void enableEncryption(const uint8_t* key16, const uint8_t* iv16);
    void disableEncryption();
  private:
    void createFile(String name, bool is_pcap, bool is_gpx = false);
    void open(bool is_pcap);
    void openFile(String file_name, fs::FS* fs, bool serial, bool is_pcap, bool is_gpx = false);
    void add(const uint8_t* buf, uint32_t len, bool is_pcap);
    void write(int32_t n);
    void write(uint32_t n);
    void write(uint16_t n);
    void write(const uint8_t* buf, uint32_t len);
    void saveFs();
    void saveSerial();

    uint8_t* bufA;
    uint8_t* bufB;

    uint32_t bufSizeA = 0;
    uint32_t bufSizeB = 0;

    bool writing = false; // acceppting writes to buffer
    bool useA = true; // writing to bufA or bufB
    bool saving = false; // currently saving onto the SD card

    String fileName = "/0.pcap";
    File file;
    fs::FS* fs;
    bool serial;

    // AES-128-CTR state
    bool              _enc_enabled = false;
    mbedtls_aes_context _enc_ctx;
    uint8_t           _enc_nonce[16];   // CTR counter (modified in-place by mbedtls)
    uint8_t           _enc_stream[16];  // cached keystream block
    size_t            _enc_nc_off;      // byte offset within current keystream block
};

#endif
