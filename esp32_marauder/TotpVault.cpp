#include "TotpVault.h"
#include "mbedtls/gcm.h"
#include "mbedtls/sha256.h"
#include <SD.h>
#include "esp_random.h"
#include <vector>

#ifdef HAS_SD
extern SDInterface sd_obj;
#endif
extern Settings settings_obj;
TotpVault totp_vault;

static void deriveKeyFromPinValue(const String &pinIn, uint8_t key[32]) {
  String pin = pinIn;
  if (pin.length() == 0) pin = "0000";
  mbedtls_sha256((const unsigned char*)pin.c_str(), pin.length(), key, 0);
}

static void deriveKeyFromSettingsPin(uint8_t key[32]) {
  deriveKeyFromPinValue(settings_obj.loadSetting<String>("PINCode"), key);
}

void TotpVault::begin(){ if(!_accounts) _accounts = new LinkedList<TotpAccount>(); }
int TotpVault::count() const { return _accounts ? _accounts->size() : 0; }
TotpAccount TotpVault::get(int i) const { return _accounts->get(i); }

bool TotpVault::addAccount(const String &label, const String &secret){
  begin();
  String cleanLabel = label;
  String cleanSecret = secret;
  cleanLabel.trim();
  cleanSecret.trim();
  if (cleanLabel.length() == 0 || cleanSecret.length() == 0) return false;
  if (_accounts->size() >= TOTP_MAX_ACCOUNTS) return false;
  for (int i = 0; i < _accounts->size(); i++) {
    TotpAccount a = _accounts->get(i);
    if (a.label == cleanLabel) return false;
  }
  _accounts->add({cleanLabel, cleanSecret});
  return true;
}

bool TotpVault::removeAccount(int index){
  if(!_accounts || index<0 || index>=_accounts->size()) return false;
  _accounts->remove(index);
  return true;
}

bool TotpVault::encrypt(const uint8_t* in, size_t inLen, uint8_t* out, size_t &outLen){
  uint8_t key[32]; deriveKeyFromSettingsPin(key);
  uint8_t iv[12];
  esp_fill_random(iv, sizeof(iv));
  uint8_t tag[16];
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
    mbedtls_gcm_free(&gcm);
    return false;
  }
  if (mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, inLen, iv, sizeof(iv), nullptr, 0, in, out + 32, sizeof(tag), tag) != 0) {
    mbedtls_gcm_free(&gcm);
    return false;
  }
  mbedtls_gcm_free(&gcm);

  memcpy(out, "TV02", 4);
  out[4] = 12; out[5] = 16;
  memset(out + 6, 0, 10);
  memcpy(out + 16, iv, 12);
  memcpy(out + 28, tag, 16);
  outLen = 32 + inLen;
  return true;
}

bool TotpVault::decrypt(const uint8_t* in, size_t inLen, uint8_t* out, size_t &outLen){
  if (inLen < 32) return false;
  if (memcmp(in, "TV02", 4) != 0) return false;
  uint8_t ivLen = in[4], tagLen = in[5];
  if (ivLen != 12 || tagLen != 16) return false;
  const uint8_t* iv = in + 16;
  const uint8_t* tag = in + 28;
  const uint8_t* ct = in + 32;
  size_t ctLen = inLen - 32;
  uint8_t key[32]; deriveKeyFromSettingsPin(key);
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
    mbedtls_gcm_free(&gcm);
    return false;
  }
  int rc = mbedtls_gcm_auth_decrypt(&gcm, ctLen, iv, ivLen, nullptr, 0, tag, tagLen, ct, out);
  mbedtls_gcm_free(&gcm);
  if (rc != 0) return false;
  outLen = ctLen;
  return true;
}


bool TotpVault::migratePinKey(const String &oldPin, const String &newPin) {
#ifdef HAS_SD
  begin();
  if(!sd_obj.supported || !SD.exists("/totp.db")) return false;
  File f = SD.open("/totp.db", FILE_READ); if(!f) return false;
  size_t len = f.size(); if(len < 32){ f.close(); return false; }
  std::vector<uint8_t> enc(len);
  if (f.read(enc.data(), len) != len) { f.close(); return false; }
  f.close();

  uint8_t keyOld[32]; deriveKeyFromPinValue(oldPin, keyOld);
  if (memcmp(enc.data(), "TV02", 4) != 0) return false;
  const uint8_t* iv = enc.data() + 16;
  const uint8_t* tag = enc.data() + 28;
  const uint8_t* ct = enc.data() + 32;
  size_t ctLen = len - 32;
  std::vector<uint8_t> dec(ctLen);
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, keyOld, 256) != 0) {
    mbedtls_gcm_free(&gcm);
    return false;
  }
  int rc = mbedtls_gcm_auth_decrypt(&gcm, ctLen, iv, 12, nullptr, 0, tag, 16, ct, dec.data());
  mbedtls_gcm_free(&gcm);
  if (rc != 0) return false;

  uint8_t keyNew[32]; deriveKeyFromPinValue(newPin, keyNew);
  uint8_t iv2[12]; esp_fill_random(iv2, sizeof(iv2));
  uint8_t tag2[16];
  std::vector<uint8_t> out(32 + ctLen);
  mbedtls_gcm_init(&gcm);
  if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, keyNew, 256) != 0) {
    mbedtls_gcm_free(&gcm);
    return false;
  }
  if (mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, ctLen, iv2, sizeof(iv2), nullptr, 0, dec.data(), out.data() + 32, sizeof(tag2), tag2) != 0) {
    mbedtls_gcm_free(&gcm);
    return false;
  }
  mbedtls_gcm_free(&gcm);
  memcpy(out.data(), "TV02", 4); out[4]=12; out[5]=16; memset(out.data()+6,0,10); memcpy(out.data()+16,iv2,12); memcpy(out.data()+28,tag2,16);

  File w = SD.open("/totp.db", FILE_WRITE); if(!w) return false;
  w.write(out.data(), 32 + ctLen); w.close();
  return true;
#else
  return false;
#endif
}

bool TotpVault::saveToSD(){
#ifdef HAS_SD
  begin();
  if (!sd_obj.supported) return false;
  DynamicJsonDocument doc(24576);
  for(int i=0;i<_accounts->size();i++){
    doc["accounts"][i]["label"] = _accounts->get(i).label;
    doc["accounts"][i]["secret"] = _accounts->get(i).secret;
  }
  String plain; serializeJson(doc, plain);
  size_t encCap = plain.length() + 64;
  std::vector<uint8_t> enc(encCap); size_t outLen=0;
  if(!encrypt((const uint8_t*)plain.c_str(), plain.length(), enc.data(), outLen)) return false;
  File f = SD.open("/totp.db", FILE_WRITE); if(!f) return false;
  f.write(enc.data(), outLen); f.close();
  return true;
#else
  return false;
#endif
}

bool TotpVault::loadFromSD(){
#ifdef HAS_SD
  begin();
  _accounts->clear();
  if(!sd_obj.supported || !SD.exists("/totp.db")) return false;
  File f = SD.open("/totp.db", FILE_READ); if(!f) return false;
  size_t len = f.size(); if(len < 32){ f.close(); return false; }
  std::vector<uint8_t> enc(len);
  if (f.read(enc.data(), len) != len) { f.close(); return false; }
  f.close();
  std::vector<uint8_t> dec(len + 1); size_t decLen=0;
  if(!decrypt(enc.data(), len, dec.data(), decLen)) return false;
  dec[decLen]='\0';
  DynamicJsonDocument doc(24576);
  if(deserializeJson(doc, (const char*)dec.data())) return false;
  for(int i=0;i<(int)doc["accounts"].size() && i<TOTP_MAX_ACCOUNTS;i++)
    _accounts->add({doc["accounts"][i]["label"].as<String>(), doc["accounts"][i]["secret"].as<String>()});
  return true;
#else
  return false;
#endif
}
