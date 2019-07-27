








#include "webrtc/test/encoder_settings.h"

#include <assert.h>
#include <string.h>

#include "webrtc/test/fake_decoder.h"
#include "webrtc/video_decoder.h"

namespace webrtc {
namespace test {
std::vector<VideoStream> CreateVideoStreams(size_t num_streams) {
  assert(num_streams > 0);

  
  static const size_t kNumSettings = 3;
  assert(num_streams <= kNumSettings);

  std::vector<VideoStream> stream_settings(kNumSettings);

  stream_settings[0].width = 320;
  stream_settings[0].height = 180;
  stream_settings[0].max_framerate = 30;
  stream_settings[0].min_bitrate_bps = 50000;
  stream_settings[0].target_bitrate_bps = stream_settings[0].max_bitrate_bps =
      150000;
  stream_settings[0].max_qp = 56;

  stream_settings[1].width = 640;
  stream_settings[1].height = 360;
  stream_settings[1].max_framerate = 30;
  stream_settings[1].min_bitrate_bps = 200000;
  stream_settings[1].target_bitrate_bps = stream_settings[1].max_bitrate_bps =
      450000;
  stream_settings[1].max_qp = 56;

  stream_settings[2].width = 1280;
  stream_settings[2].height = 720;
  stream_settings[2].max_framerate = 30;
  stream_settings[2].min_bitrate_bps = 700000;
  stream_settings[2].target_bitrate_bps = stream_settings[2].max_bitrate_bps =
      1500000;
  stream_settings[2].max_qp = 56;
  stream_settings.resize(num_streams);
  return stream_settings;
}

VideoReceiveStream::Decoder CreateMatchingDecoder(
    const VideoSendStream::Config::EncoderSettings& encoder_settings) {
  VideoReceiveStream::Decoder decoder;
  decoder.payload_type = encoder_settings.payload_type;
  decoder.payload_name = encoder_settings.payload_name;
  if (encoder_settings.payload_name == "VP8") {
    decoder.decoder = VideoDecoder::Create(VideoDecoder::kVp8);
  } else if (encoder_settings.payload_name == "VP9") {
    decoder.decoder = VideoDecoder::Create(VideoDecoder::kVp9);
  } else {
    decoder.decoder = new FakeDecoder();
  }
  return decoder;
}
}  
}  
