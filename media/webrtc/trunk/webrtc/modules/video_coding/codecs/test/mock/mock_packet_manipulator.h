









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_MOCK_MOCK_PACKET_MANIPULATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_MOCK_MOCK_PACKET_MANIPULATOR_H_

#include "modules/video_coding/codecs/test/packet_manipulator.h"

#include <string>

#include "common_video/interface/video_image.h"
#include "gmock/gmock.h"
#include "typedefs.h"

namespace webrtc {
namespace test {

class MockPacketManipulator : public PacketManipulator {
 public:
  MOCK_METHOD1(ManipulatePackets, int(webrtc::EncodedImage* encoded_image));
};

}  
}  

#endif  
