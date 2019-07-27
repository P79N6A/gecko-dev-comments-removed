








#ifndef WEBRTC_TEST_ENCODER_SETTINGS_H_
#define WEBRTC_TEST_ENCODER_SETTINGS_H_

#include "webrtc/video_receive_stream.h"
#include "webrtc/video_send_stream.h"

namespace webrtc {
namespace test {
std::vector<VideoStream> CreateVideoStreams(size_t num_streams);

VideoReceiveStream::Decoder CreateMatchingDecoder(
    const VideoSendStream::Config::EncoderSettings& encoder_settings);
}  
}  

#endif  
