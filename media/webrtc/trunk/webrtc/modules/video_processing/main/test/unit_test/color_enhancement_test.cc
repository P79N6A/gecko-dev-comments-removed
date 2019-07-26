









#include <stdio.h>
#include <stdlib.h>

#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/modules/video_processing/main/test/unit_test/video_processing_unittest.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {

TEST_F(VideoProcessingModuleTest, ColorEnhancement)
{
    TickTime t0;
    TickTime t1;
    TickInterval accTicks;

    
    fclose(_sourceFile);
    const std::string video_file =
      webrtc::test::ResourcePath("foreman_cif_short", "yuv");
    _sourceFile  = fopen(video_file.c_str(), "rb");
    ASSERT_TRUE(_sourceFile != NULL) <<
        "Cannot read source file: " + video_file + "\n";

    std::string output_file = webrtc::test::OutputPath() +
        "foremanColorEnhancedVPM_cif_short.yuv";
    FILE* modFile = fopen(output_file.c_str(), "w+b");
    ASSERT_TRUE(modFile != NULL) << "Could not open output file.\n";

    uint32_t frameNum = 0;
    scoped_array<uint8_t> video_buffer(new uint8_t[_frame_length]);
    while (fread(video_buffer.get(), 1, _frame_length, _sourceFile) ==
        _frame_length)
    {
        
        EXPECT_EQ(0, ConvertToI420(kI420, video_buffer.get(), 0, 0,
                                   _width, _height,
                                   0, kRotateNone, &_videoFrame));
        frameNum++;
        t0 = TickTime::Now();
        ASSERT_EQ(0, VideoProcessingModule::ColorEnhancement(&_videoFrame));
        t1 = TickTime::Now();
        accTicks += t1 - t0;
        if (PrintI420VideoFrame(_videoFrame, modFile) < 0) {
          return;
        }
    }
    ASSERT_NE(0, feof(_sourceFile)) << "Error reading source file";

    printf("\nTime per frame: %d us \n",
        static_cast<int>(accTicks.Microseconds() / frameNum));
    rewind(modFile);

    printf("Comparing files...\n\n");
    std::string reference_filename =
        webrtc::test::ResourcePath("foremanColorEnhanced_cif_short", "yuv");
    FILE* refFile = fopen(reference_filename.c_str(), "rb");
    ASSERT_TRUE(refFile != NULL) << "Cannot open reference file: " <<
        reference_filename << "\n"
        "Create the reference by running Matlab script createTable.m.";

    
    ASSERT_EQ(0, fseek(refFile, 0L, SEEK_END));
    long refLen = ftell(refFile);
    ASSERT_NE(-1L, refLen);
    rewind(refFile);
    ASSERT_EQ(0, fseek(modFile, 0L, SEEK_END));
    long testLen = ftell(modFile);
    ASSERT_NE(-1L, testLen);
    rewind(modFile);
    ASSERT_EQ(refLen, testLen) << "File lengths differ.";

    I420VideoFrame refVideoFrame;
    refVideoFrame.CreateEmptyFrame(_width, _height,
                                   _width, _half_width, _half_width);

    
    scoped_array<uint8_t> ref_buffer(new uint8_t[_frame_length]);
    while (fread(video_buffer.get(), 1, _frame_length, modFile) ==
        _frame_length)
    {
        
        EXPECT_EQ(0, ConvertToI420(kI420, video_buffer.get(), 0, 0,
                                   _width, _height,
                                   0, kRotateNone, &_videoFrame));
        ASSERT_EQ(_frame_length, fread(ref_buffer.get(), 1, _frame_length,
                                       refFile));
        EXPECT_EQ(0, ConvertToI420(kI420, ref_buffer.get(), 0, 0,
                                   _width, _height,
                                    0, kRotateNone, &refVideoFrame));
        EXPECT_EQ(0, memcmp(_videoFrame.buffer(kYPlane),
                            refVideoFrame.buffer(kYPlane),
                            _size_y));
        EXPECT_EQ(0, memcmp(_videoFrame.buffer(kUPlane),
                            refVideoFrame.buffer(kUPlane),
                            _size_uv));
        EXPECT_EQ(0, memcmp(_videoFrame.buffer(kVPlane),
                            refVideoFrame.buffer(kVPlane),
                            _size_uv));
    }
    ASSERT_NE(0, feof(_sourceFile)) << "Error reading source file";

    
    

    scoped_array<uint8_t> testFrame(new uint8_t[_frame_length]);

    
    
    memset(testFrame.get(), 128, _frame_length);

    I420VideoFrame testVideoFrame;
    testVideoFrame.CreateEmptyFrame(_width, _height,
                                    _width, _half_width, _half_width);
    EXPECT_EQ(0, ConvertToI420(kI420, testFrame.get(), 0, 0,
                               _width, _height, 0, kRotateNone,
                               &testVideoFrame));

    ASSERT_EQ(0, VideoProcessingModule::ColorEnhancement(&testVideoFrame));

    EXPECT_EQ(0, memcmp(testVideoFrame.buffer(kYPlane), testFrame.get(),
                        _size_y))
      << "Function is modifying the luminance.";

    EXPECT_NE(0, memcmp(testVideoFrame.buffer(kUPlane),
                        testFrame.get() + _size_y, _size_uv)) <<
                        "Function is not modifying all chrominance pixels";
    EXPECT_NE(0, memcmp(testVideoFrame.buffer(kVPlane),
                        testFrame.get() + _size_y + _size_uv, _size_uv)) <<
                        "Function is not modifying all chrominance pixels";

    ASSERT_EQ(0, fclose(refFile));
    ASSERT_EQ(0, fclose(modFile));
}

}  
