#include "TotpUtil.h"
#include "mbedtls/md.h"
#include <time.h>

static int b32Val(char c){
  if(c>='A'&&c<='Z') return c-'A';
  if(c>='a'&&c<='z') return c-'a';
  if(c>='2'&&c<='7') return c-'2'+26;
  return -1;
}

static size_t decodeBase32(const String &in, uint8_t *out, size_t outMax){
  int buffer=0,bitsLeft=0; size_t outLen=0;
  for(size_t i=0;i<in.length();i++){
    char c=in[i]; if(c=='='||c==' ') continue;
    int v=b32Val(c); if(v<0) continue;
    buffer=(buffer<<5)|v; bitsLeft+=5;
    if(bitsLeft>=8){
      bitsLeft-=8;
      if(outLen>=outMax) return 0;
      out[outLen++]=(buffer>>bitsLeft)&0xFF;
    }
  }
  return outLen;
}

bool hasValidSystemTime(){
  time_t now=time(nullptr);
  return now > 1700000000; // after Nov 2023
}

String generateTotpCode(const String &base32Secret, uint32_t unixTime, uint32_t period, uint8_t digits){
  if(base32Secret.length()==0 || period==0 || digits<6) return "------";
  uint8_t key[64];
  size_t keyLen = decodeBase32(base32Secret, key, sizeof(key));
  if(keyLen==0) return "------";

  uint64_t counter = unixTime / period;
  uint8_t msg[8];
  for(int i=7;i>=0;i--){ msg[i]=counter & 0xFF; counter >>= 8; }

  uint8_t hmac[20];
  const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  if(!md) return "------";
  if(mbedtls_md_hmac(md, key, keyLen, msg, sizeof(msg), hmac)!=0) return "------";

  int offset = hmac[19] & 0x0F;
  uint32_t bin = ((hmac[offset] & 0x7F) << 24) | ((hmac[offset+1] & 0xFF) << 16) |
                 ((hmac[offset+2] & 0xFF) << 8) | (hmac[offset+3] & 0xFF);
  uint32_t mod = 1; for(uint8_t i=0;i<digits;i++) mod *= 10;
  uint32_t otp = bin % mod;
  char buf[12];
  snprintf(buf, sizeof(buf), "%0*u", digits, otp);
  return String(buf);
}
