









#include "webrtc/video/encoded_frame_callback_adapter.h"

#include "webrtc/modules/video_coding/main/source/encoded_frame.h"

namespace webrtc {
namespace internal {

EncodedFrameCallbackAdapter::EncodedFrameCallbackAdapter(
    EncodedFrameObserver* observer) : observer_(observer) {
}

EncodedFrameCallbackAdapter::~EncodedFrameCallbackAdapter() {}

int32_t EncodedFrameCallbackAdapter::Encoded(
    EncodedImage& encodedImage,
    const CodecSpecificInfo* codecSpecificInfo,
    const RTPFragmentationHeader* fragmentation) {
  assert(observer_ != NULL);
  FrameType frame_type =
        VCMEncodedFrame::ConvertFrameType(encodedImage._frameType);
  const EncodedFrame frame(encodedImage._buffer,
                           encodedImage._length,
                           frame_type);
  observer_->EncodedFrameCallback(frame);
  return 0;
}

}  
}  
