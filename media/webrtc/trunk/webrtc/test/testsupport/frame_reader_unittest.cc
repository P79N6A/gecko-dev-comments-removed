









#include "webrtc/test/testsupport/frame_reader.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {
namespace test {

const std::string kInputFilename = "temp_inputfile.tmp";
const std::string kInputFileContents = "baz";


const size_t kFrameLength = 1000;

class FrameReaderTest: public testing::Test {
 protected:
  FrameReaderTest() {}
  virtual ~FrameReaderTest() {}
  void SetUp() {
    
    remove(kInputFilename.c_str());

    
    FILE* dummy = fopen(kInputFilename.c_str(), "wb");
    fprintf(dummy, "%s", kInputFileContents.c_str());
    fclose(dummy);

    frame_reader_ = new FrameReaderImpl(kInputFilename, kFrameLength);
    ASSERT_TRUE(frame_reader_->Init());
  }
  void TearDown() {
    delete frame_reader_;
    
    remove(kInputFilename.c_str());
  }
  FrameReader* frame_reader_;
};

TEST_F(FrameReaderTest, InitSuccess) {
  FrameReaderImpl frame_reader(kInputFilename, kFrameLength);
  ASSERT_TRUE(frame_reader.Init());
  ASSERT_EQ(kFrameLength, frame_reader.FrameLength());
  ASSERT_EQ(0, frame_reader.NumberOfFrames());
}

TEST_F(FrameReaderTest, ReadFrame) {
  uint8_t buffer[3];
  bool result = frame_reader_->ReadFrame(buffer);
  ASSERT_FALSE(result);  
  ASSERT_EQ(kInputFileContents[0], buffer[0]);
  ASSERT_EQ(kInputFileContents[1], buffer[1]);
  ASSERT_EQ(kInputFileContents[2], buffer[2]);
}

TEST_F(FrameReaderTest, ReadFrameUninitialized) {
  uint8_t buffer[3];
  FrameReaderImpl file_reader(kInputFilename, kFrameLength);
  ASSERT_FALSE(file_reader.ReadFrame(buffer));
}

}  
}  
