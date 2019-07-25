









#include "testsupport/frame_writer.h"

#include "gtest/gtest.h"
#include "testsupport/fileutils.h"

namespace webrtc {
namespace test {

const std::string kOutputFilename = "temp_outputfile.tmp";
const int kFrameLength = 1000;

class FrameWriterTest: public testing::Test {
 protected:
  FrameWriterTest() {}
  virtual ~FrameWriterTest() {}
  void SetUp() {
    
    std::remove(kOutputFilename.c_str());
    frame_writer_ = new FrameWriterImpl(kOutputFilename, kFrameLength);
    ASSERT_TRUE(frame_writer_->Init());
  }
  void TearDown() {
    delete frame_writer_;
    
    std::remove(kOutputFilename.c_str());
  }
  FrameWriter* frame_writer_;
};

TEST_F(FrameWriterTest, InitSuccess) {
  FrameWriterImpl frame_writer(kOutputFilename, kFrameLength);
  ASSERT_TRUE(frame_writer.Init());
  ASSERT_EQ(kFrameLength, frame_writer.FrameLength());
}

TEST_F(FrameWriterTest, WriteFrame) {
  WebRtc_UWord8 buffer[kFrameLength];
  memset(buffer, 9, kFrameLength);  
  bool result = frame_writer_->WriteFrame(buffer);
  ASSERT_TRUE(result);  
  
  frame_writer_->Close();
  ASSERT_EQ(kFrameLength,
            static_cast<int>(GetFileSize(kOutputFilename)));
}

TEST_F(FrameWriterTest, WriteFrameUninitialized) {
  WebRtc_UWord8 buffer[3];
  FrameWriterImpl frame_writer(kOutputFilename, kFrameLength);
  ASSERT_FALSE(frame_writer.WriteFrame(buffer));
}

}  
}  
