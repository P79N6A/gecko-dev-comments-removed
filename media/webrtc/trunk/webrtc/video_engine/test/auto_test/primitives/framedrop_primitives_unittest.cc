









#include "framedrop_primitives.h"

#include <cstdio>
#include <vector>

#include "gtest/gtest.h"
#include "testsupport/fileutils.h"
#include "testsupport/frame_reader.h"
#include "testsupport/frame_writer.h"

namespace webrtc {

const std::string kOutputFilename = "temp_outputfile.tmp";
const int kFrameLength = 1000;

class FrameDropPrimitivesTest: public testing::Test {
 protected:
  FrameDropPrimitivesTest() {}
  virtual ~FrameDropPrimitivesTest() {}
  void SetUp() {
    
    std::remove(kOutputFilename.c_str());
  }
  void TearDown() {
    
    std::remove(kOutputFilename.c_str());
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

  
  WebRtc_UWord8 first_frame_data[kFrameLength];
  memset(first_frame_data, 5, kFrameLength);  
  WebRtc_UWord8 third_frame_data[kFrameLength];
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
  WebRtc_UWord8 read_buffer[kFrameLength];
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
