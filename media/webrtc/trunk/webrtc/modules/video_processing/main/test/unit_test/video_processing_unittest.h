









#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_TEST_UNIT_TEST_VIDEO_PROCESSING_UNITTEST_H
#define WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_TEST_UNIT_TEST_VIDEO_PROCESSING_UNITTEST_H

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {

class VideoProcessingModuleTest : public ::testing::Test {
 protected:
  VideoProcessingModuleTest();
  virtual void SetUp();
  virtual void TearDown();
  static void SetUpTestCase() {
    Trace::CreateTrace();
    std::string trace_file = webrtc::test::OutputPath() + "VPMTrace.txt";
    ASSERT_EQ(0, Trace::SetTraceFile(trace_file.c_str()));
  }
  static void TearDownTestCase() {
    Trace::ReturnTrace();
  }
  VideoProcessingModule* vpm_;
  FILE* source_file_;
  I420VideoFrame video_frame_;
  const int width_;
  const int half_width_;
  const int height_;
  const int size_y_;
  const int size_uv_;
  const unsigned int frame_length_;
};

}  

#endif 
