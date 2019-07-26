









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_MOCK_MOCK_PACKET_MANIPULATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_MOCK_MOCK_PACKET_MANIPULATOR_H_

#include "webrtc/modules/video_coding/codecs/test/packet_manipulator.h"

#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "webrtc/common_video/interface/video_image.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class MockPacketManipulator : public PacketManipulator {
 public:
  MOCK_METHOD1(ManipulatePackets, int(webrtc::EncodedImage* encoded_image));
};

}  
}  

#endif  
