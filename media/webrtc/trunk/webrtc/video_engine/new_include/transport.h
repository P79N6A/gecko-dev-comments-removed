









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_TRANSPORT_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_TRANSPORT_H_

#include <stddef.h>

namespace webrtc {
namespace newapi {

class Transport {
 public:
  virtual bool SendRTP(const void* packet, size_t length) = 0;
  virtual bool SendRTCP(const void* packet, size_t length) = 0;

 protected:
  virtual ~Transport() {}
};
}  
}  

#endif  
