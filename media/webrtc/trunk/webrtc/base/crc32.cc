









#include "webrtc/base/crc32.h"

#include "webrtc/base/basicdefs.h"

namespace rtc {





static const uint32 kCrc32Polynomial = 0xEDB88320;
static uint32 kCrc32Table[256] = { 0 };

static void EnsureCrc32TableInited() {
  if (kCrc32Table[ARRAY_SIZE(kCrc32Table) - 1])
    return;  
  for (uint32 i = 0; i < ARRAY_SIZE(kCrc32Table); ++i) {
    uint32 c = i;
    for (size_t j = 0; j < 8; ++j) {
      if (c & 1) {
        c = kCrc32Polynomial ^ (c >> 1);
      } else {
        c >>= 1;
      }
    }
    kCrc32Table[i] = c;
  }
}

uint32 UpdateCrc32(uint32 start, const void* buf, size_t len) {
  EnsureCrc32TableInited();

  uint32 c = start ^ 0xFFFFFFFF;
  const uint8* u = static_cast<const uint8*>(buf);
  for (size_t i = 0; i < len; ++i) {
    c = kCrc32Table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
  }
  return c ^ 0xFFFFFFFF;
}

}  

