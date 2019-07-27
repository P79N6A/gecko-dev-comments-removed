









#ifndef WEBRTC_BASE_CRC32_H_
#define WEBRTC_BASE_CRC32_H_

#include <string>

#include "webrtc/base/basictypes.h"

namespace rtc {



uint32 UpdateCrc32(uint32 initial, const void* buf, size_t len);


inline uint32 ComputeCrc32(const void* buf, size_t len) {
  return UpdateCrc32(0, buf, len);
}
inline uint32 ComputeCrc32(const std::string& str) {
  return ComputeCrc32(str.c_str(), str.size());
}

}  

#endif  
