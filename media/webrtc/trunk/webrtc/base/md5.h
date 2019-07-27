




















#ifndef WEBRTC_BASE_MD5_H_
#define WEBRTC_BASE_MD5_H_

#include "webrtc/base/basictypes.h"

namespace rtc {


typedef struct MD5Context MD5_CTX;

struct MD5Context {
  uint32 buf[4];
  uint32 bits[2];
  uint32 in[16];
};

void MD5Init(MD5Context* context);
void MD5Update(MD5Context* context, const uint8* data, size_t len);
void MD5Final(MD5Context* context, uint8 digest[16]);
void MD5Transform(uint32 buf[4], const uint32 in[16]);

}  

#endif  
