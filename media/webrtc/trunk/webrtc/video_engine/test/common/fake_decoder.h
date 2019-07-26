









#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_FAKE_DECODER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_FAKE_DECODER_H_

#include <vector>

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {
namespace test {

class FakeDecoder : public VideoDecoder {
 public:
  FakeDecoder();

  virtual int32_t InitDecode(const VideoCodec* config,
                             int32_t number_of_cores) OVERRIDE;

  virtual int32_t Decode(const EncodedImage& input,
                         bool missing_frames,
                         const RTPFragmentationHeader* fragmentation,
                         const CodecSpecificInfo* codec_specific_info,
                         int64_t render_time_ms) OVERRIDE;

  virtual int32_t RegisterDecodeCompleteCallback(
      DecodedImageCallback* callback) OVERRIDE;

  virtual int32_t Release() OVERRIDE;
  virtual int32_t Reset() OVERRIDE;

 private:
  VideoCodec config_;
  I420VideoFrame frame_;
  DecodedImageCallback* callback_;
};
}  
}  

#endif  
