












#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_ENCRYPTION_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_ENCRYPTION_H_

#include "common_types.h"

namespace webrtc {
class VideoEngine;

class WEBRTC_DLLEXPORT ViEEncryption {
 public:
  
  
  
  static ViEEncryption* GetInterface(VideoEngine* video_engine);

  
  
  
  
  virtual int Release() = 0;

  
  
  virtual int RegisterExternalEncryption(const int video_channel,
                                         Encryption& encryption) = 0;

  
  
  virtual int DeregisterExternalEncryption(const int video_channel) = 0;

 protected:
  ViEEncryption() {}
  virtual ~ViEEncryption() {}
};

}  

#endif  
