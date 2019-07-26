









#include "webrtc/video_engine/test/auto_test/primitives/framedrop_primitives.h"

#include <stdio.h>

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/frame_reader.h"
#include "webrtc/test/testsupport/frame_writer.h"

namespace webrtc {

const std::string kOutputFilename = "temp_outputfile.tmp";
const int kFrameLength = 1000;

class FrameDropPrimitivesTest: public testing::Test {
 protected:
  FrameDropPrimitivesTest() {}
  virtual ~FrameDropPrimitivesTest() {}
  void SetUp() {
    
    remove(kOutputFilename.c_str());
  }
  void TearDown() {
    
    remove(kOutputFilename.c_str());
  }
};

TEST_F(FrameDropPrimitivesTest, FixOutputFileForComparison) {
  
  
  std::vector<Frame*> frames;
  Frame first_frame(0, kFrameLength);
  Frame second_frame(0, kFrameLength);
  Frame third_frame(0, kFrameLength);
  Frame fourth_frame(0, kFrameLength);

  second_frame.dropped_at_render = true;
  fourth_frame.dropped_at_render = true;

  frames.push_back(&first_frame);
  frames.push_back(&second_frame);
  frames.push_back(&third_frame);
  frames.push_back(&fourth_frame);

  
  uint8_t first_frame_data[kFrameLength];
  memset(first_frame_data, 5, kFrameLength);  
  uint8_t third_frame_data[kFrameLength];
  memset(third_frame_data, 7, kFrameLength);  

  
  
  
  webrtc::test::FrameWriterImpl frame_writer(kOutputFilename, kFrameLength);
  EXPECT_TRUE(frame_writer.Init());
  EXPECT_TRUE(frame_writer.WriteFrame(first_frame_data));
  EXPECT_TRUE(frame_writer.WriteFrame(third_frame_data));
  frame_writer.Close();
  EXPECT_EQ(2 * kFrameLength,
            static_cast<int>(webrtc::test::GetFileSize(kOutputFilename)));

  FixOutputFileForComparison(kOutputFilename, kFrameLength, frames);

  
  EXPECT_EQ(4 * kFrameLength,
            static_cast<int>(webrtc::test::GetFileSize(kOutputFilename)));

  webrtc::test::FrameReaderImpl frame_reader(kOutputFilename, kFrameLength);
  frame_reader.Init();
  uint8_t read_buffer[kFrameLength];
  EXPECT_TRUE(frame_reader.ReadFrame(read_buffer));
  EXPECT_EQ(0, memcmp(read_buffer, first_frame_data, kFrameLength));
  EXPECT_TRUE(frame_reader.ReadFrame(read_buffer));
  EXPECT_EQ(0, memcmp(read_buffer, first_frame_data, kFrameLength));

  EXPECT_TRUE(frame_reader.ReadFrame(read_buffer));
  EXPECT_EQ(0, memcmp(read_buffer, third_frame_data, kFrameLength));
  EXPECT_TRUE(frame_reader.ReadFrame(read_buffer));
  EXPECT_EQ(0, memcmp(read_buffer, third_frame_data, kFrameLength));

  frame_reader.Close();
}

}  
