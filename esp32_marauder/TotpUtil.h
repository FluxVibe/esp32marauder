#pragma once
#include "configs.h"
#include <Arduino.h>

String generateTotpCode(const String &base32Secret, uint32_t unixTime, uint32_t period = 30, uint8_t digits = 6);
bool hasValidSystemTime();
