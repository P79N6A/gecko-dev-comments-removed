















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_IMAGE_PROCESS_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_IMAGE_PROCESS_H_

#include "common_types.h"
#include "common_video/interface/i420_video_frame.h"

namespace webrtc {

class VideoEngine;





class WEBRTC_DLLEXPORT ViEEffectFilter {
 public:
  
  
  virtual int Transform(int size, unsigned char* frameBuffer,
                        unsigned int timeStamp90KHz, unsigned int width,
                        unsigned int height) = 0;
 protected:
  ViEEffectFilter() {}
  virtual ~ViEEffectFilter() {}
};

class WEBRTC_DLLEXPORT ViEImageProcess {
 public:
  
  
  
  static ViEImageProcess* GetInterface(VideoEngine* video_engine);

  
  
  
  virtual int Release() = 0;

  
  
  virtual int RegisterCaptureEffectFilter(const int capture_id,
                                          ViEEffectFilter& capture_filter) = 0;

  
  virtual int DeregisterCaptureEffectFilter(const int capture_id) = 0;

  
  virtual int RegisterSendEffectFilter(const int video_channel,
                                       ViEEffectFilter& send_filter) = 0;

  
  virtual int DeregisterSendEffectFilter(const int video_channel) = 0;

  
  
  virtual int RegisterRenderEffectFilter(const int video_channel,
                                         ViEEffectFilter& render_filter) = 0;

  
  virtual int DeregisterRenderEffectFilter(const int video_channel) = 0;

  
  
  
  
  virtual int EnableDeflickering(const int capture_id, const bool enable) = 0;

  
  
  virtual int EnableDenoising(const int capture_id, const bool enable) = 0;

  
  
  virtual int EnableColorEnhancement(const int video_channel,
                                     const bool enable) = 0;

 protected:
  ViEImageProcess() {}
  virtual ~ViEImageProcess() {}
};

}  

#endif  
