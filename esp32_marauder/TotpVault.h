#pragma once

#include "configs.h"
#include "settings.h"
#ifdef HAS_SD
#include "SDInterface.h"
#endif
#include <ArduinoJson.h>
#include <LinkedList.h>

#define TOTP_MAX_ACCOUNTS 100

struct TotpAccount {
  String label;
  String secret;
};

class TotpVault {
public:
  void begin();
  int count() const;
  TotpAccount get(int index) const;
  bool addAccount(const String &label, const String &secret);
  bool removeAccount(int index);
  bool saveToSD();
  bool loadFromSD();
  bool migratePinKey(const String &oldPin, const String &newPin);
private:
  LinkedList<TotpAccount>* _accounts = nullptr;
  bool encrypt(const uint8_t* in, size_t inLen, uint8_t* out, size_t &outLen);
  bool decrypt(const uint8_t* in, size_t inLen, uint8_t* out, size_t &outLen);
};

extern TotpVault totp_vault;
