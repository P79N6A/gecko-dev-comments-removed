









#ifndef WEBRTC_TEST_TESTSUPPORT_MOCK_MOCK_FRAME_WRITER_H_
#define WEBRTC_TEST_TESTSUPPORT_MOCK_MOCK_FRAME_WRITER_H_

#include "testsupport/frame_writer.h"

#include "gmock/gmock.h"

namespace webrtc {
namespace test {

class MockFrameWriter : public FrameWriter {
 public:
  MOCK_METHOD0(Init, bool());
  MOCK_METHOD1(WriteFrame, bool(WebRtc_UWord8* frame_buffer));
  MOCK_METHOD0(Close, void());
  MOCK_METHOD0(FrameLength, int());
};

}  
}  

#endif  
