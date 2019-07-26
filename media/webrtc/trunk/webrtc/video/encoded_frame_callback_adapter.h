









#ifndef WEBRTC_VIDEO_ENCODED_FRAME_CALLBACK_ADAPTER_H_
#define WEBRTC_VIDEO_ENCODED_FRAME_CALLBACK_ADAPTER_H_

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/frame_callback.h"

namespace webrtc {
namespace internal {

class EncodedFrameCallbackAdapter : public EncodedImageCallback {
 public:
  explicit EncodedFrameCallbackAdapter(EncodedFrameObserver* observer);
  virtual ~EncodedFrameCallbackAdapter();

  virtual int32_t Encoded(EncodedImage& encodedImage,
                          const CodecSpecificInfo* codecSpecificInfo,
                          const RTPFragmentationHeader* fragmentation);

 private:
  EncodedFrameObserver* observer_;
};

}  
}  

#endif  
