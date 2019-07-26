









#ifndef WEBRTC_VIDEO_ENGINE_VIE_EXTERNAL_CODEC_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_EXTERNAL_CODEC_IMPL_H_

#include "video_engine/include/vie_external_codec.h"
#include "video_engine/vie_ref_count.h"

namespace webrtc {

class ViESharedData;

class ViEExternalCodecImpl
    : public ViEExternalCodec,
      public ViERefCount {
 public:
  
  virtual int Release();
  virtual int RegisterExternalSendCodec(const int video_channel,
                                        const unsigned char pl_type,
                                        VideoEncoder* encoder);
  virtual int DeRegisterExternalSendCodec(const int video_channel,
                                          const unsigned char pl_type);
  virtual int RegisterExternalReceiveCodec(const int video_channel,
                                           const unsigned int pl_type,
                                           VideoDecoder* decoder,
                                           bool decoder_render = false,
                                           int render_delay = 0);
  virtual int DeRegisterExternalReceiveCodec(const int video_channel,
                                             const unsigned char pl_type);

 protected:
  explicit ViEExternalCodecImpl(ViESharedData* shared_data);
  virtual ~ViEExternalCodecImpl();

 private:
  ViESharedData* shared_data_;
};

}  

#endif  
