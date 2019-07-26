









#ifndef WEBRTC_TEST_TESTSUPPORT_MOCK_MOCK_FRAME_READER_H_
#define WEBRTC_TEST_TESTSUPPORT_MOCK_MOCK_FRAME_READER_H_

#include "testsupport/frame_reader.h"

#include "gmock/gmock.h"

namespace webrtc {
namespace test {

class MockFrameReader : public FrameReader {
 public:
  MOCK_METHOD0(Init, bool());
  MOCK_METHOD1(ReadFrame, bool(uint8_t* source_buffer));
  MOCK_METHOD0(Close, void());
  MOCK_METHOD0(FrameLength, size_t());
  MOCK_METHOD0(NumberOfFrames, int());
};

}  
}  

#endif  
