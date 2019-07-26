















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_IMAGE_PROCESS_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_IMAGE_PROCESS_H_

#include "webrtc/common_types.h"

namespace webrtc {

class EncodedImageCallback;
class I420FrameCallback;
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

  
  virtual void RegisterPreEncodeCallback(
      int video_channel,
      I420FrameCallback* pre_encode_callback) = 0;
  virtual void DeRegisterPreEncodeCallback(int video_channel) = 0;

  virtual void RegisterPostEncodeImageCallback(
      int video_channel,
      EncodedImageCallback* post_encode_callback) {}
  virtual void DeRegisterPostEncodeCallback(int video_channel) {}

  virtual void RegisterPreDecodeImageCallback(
      int video_channel,
      EncodedImageCallback* pre_decode_callback) {}
  virtual void DeRegisterPreDecodeCallback(int video_channel) {}

  virtual void RegisterPreRenderCallback(
      int video_channel,
      I420FrameCallback* pre_render_callback) = 0;
  virtual void DeRegisterPreRenderCallback(int video_channel) = 0;

 protected:
  ViEImageProcess() {}
  virtual ~ViEImageProcess() {}
};

}  

#endif  
