









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_TRANSPORT_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_TRANSPORT_H_

#include <stddef.h>

#include "webrtc/typedefs.h"

namespace webrtc {
namespace newapi {

class Transport {
 public:
  virtual bool SendRtp(const uint8_t* packet, size_t length) = 0;
  virtual bool SendRtcp(const uint8_t* packet, size_t length) = 0;

 protected:
  virtual ~Transport() {}
};
}  
}  

#endif  
