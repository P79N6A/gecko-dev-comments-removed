










#include "webrtc/modules/video_coding/codecs/vp8/vp8_impl.h"

namespace webrtc {

VP8Encoder* VP8Encoder::Create() {
  return new VP8EncoderImpl();
}

VP8Decoder* VP8Decoder::Create() {
  return new VP8DecoderImpl();
}

}  
