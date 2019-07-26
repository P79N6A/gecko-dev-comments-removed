









#ifndef WEBRTC_VIDEO_ENGINE_VIE_IMAGE_PROCESS_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_IMAGE_PROCESS_IMPL_H_

#include "webrtc/typedefs.h"
#include "webrtc/video_engine/include/vie_image_process.h"
#include "webrtc/video_engine/vie_ref_count.h"

namespace webrtc {

class ViESharedData;

class ViEImageProcessImpl
    : public ViEImageProcess,
      public ViERefCount {
 public:
  
  virtual int Release();
  virtual int RegisterCaptureEffectFilter(const int capture_id,
                                          ViEEffectFilter& capture_filter);
  virtual int DeregisterCaptureEffectFilter(const int capture_id);
  virtual int RegisterSendEffectFilter(const int video_channel,
                                       ViEEffectFilter& send_filter);
  virtual int DeregisterSendEffectFilter(const int video_channel);
  virtual int RegisterRenderEffectFilter(const int video_channel,
                                         ViEEffectFilter& render_filter);
  virtual int DeregisterRenderEffectFilter(const int video_channel);
  virtual int EnableDeflickering(const int capture_id, const bool enable);
  virtual int EnableDenoising(const int capture_id, const bool enable);
  virtual int EnableColorEnhancement(const int video_channel,
                                     const bool enable);
  virtual void RegisterPreEncodeCallback(
      int video_channel,
      I420FrameCallback* pre_encode_callback) OVERRIDE;
  virtual void DeRegisterPreEncodeCallback(int video_channel) OVERRIDE;

  virtual void RegisterPostEncodeImageCallback(
      int video_channel,
      EncodedImageCallback* post_encode_callback) OVERRIDE;
  virtual void DeRegisterPostEncodeCallback(int video_channel) OVERRIDE;

  virtual void RegisterPreDecodeImageCallback(
        int video_channel,
        EncodedImageCallback* post_encode_callback) OVERRIDE;
  virtual void DeRegisterPreDecodeCallback(int video_channel) OVERRIDE;

  virtual void RegisterPreRenderCallback(
      int video_channel,
      I420FrameCallback* pre_render_callback) OVERRIDE;
  virtual void DeRegisterPreRenderCallback(int video_channel) OVERRIDE;

 protected:
  explicit ViEImageProcessImpl(ViESharedData* shared_data);
  virtual ~ViEImageProcessImpl();

 private:
  ViESharedData* shared_data_;
};

}  

#endif  
