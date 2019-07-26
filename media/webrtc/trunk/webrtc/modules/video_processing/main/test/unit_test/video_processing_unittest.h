









#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_TEST_UNIT_TEST_VIDEO_PROCESSING_UNITTEST_H
#define WEBRTC_MODULES_VIDEO_PROCESSING_MAIN_TEST_UNIT_TEST_VIDEO_PROCESSING_UNITTEST_H

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {

class VideoProcessingModuleTest : public ::testing::Test
{
protected:
    VideoProcessingModuleTest();
    virtual void SetUp();
    virtual void TearDown();
    static void SetUpTestCase()
    {
      Trace::CreateTrace();
      std::string trace_file = webrtc::test::OutputPath() + "VPMTrace.txt";
      ASSERT_EQ(0, Trace::SetTraceFile(trace_file.c_str()));
    }
    static void TearDownTestCase()
    {
      Trace::ReturnTrace();
    }
    VideoProcessingModule* _vpm;
    FILE* _sourceFile;
    I420VideoFrame _videoFrame;
    const int _width;
    const int _half_width;
    const int _height;
    const int _size_y;
    const int _size_uv;
    const unsigned int _frame_length;
};

}  

#endif 
