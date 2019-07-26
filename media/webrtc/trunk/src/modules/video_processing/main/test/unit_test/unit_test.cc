









#include "modules/video_processing/main/test/unit_test/unit_test.h"

#include <string>

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "system_wrappers/interface/tick_util.h"
#include "testsupport/fileutils.h"

namespace webrtc {






void TestSize(const VideoFrame& sourceFrame, int target_width,
              int target_height, int mode, double expected_psnr,
              VideoProcessingModule* vpm);

VideoProcessingModuleTest::VideoProcessingModuleTest() :
  _vpm(NULL),
  _sourceFile(NULL),
  _width(352),
  _height(288),
  _frameLength(CalcBufferSize(kI420, 352, 288))
{
}

void VideoProcessingModuleTest::SetUp()
{
  _vpm = VideoProcessingModule::Create(0);
  ASSERT_TRUE(_vpm != NULL);

  ASSERT_EQ(0, _videoFrame.VerifyAndAllocate(_frameLength));
  _videoFrame.SetWidth(_width);
  _videoFrame.SetHeight(_height);

  const std::string video_file =
      webrtc::test::ResourcePath("foreman_cif", "yuv");
  _sourceFile  = fopen(video_file.c_str(),"rb");
  ASSERT_TRUE(_sourceFile != NULL) <<
      "Cannot read source file: " + video_file + "\n";
}

void VideoProcessingModuleTest::TearDown()
{
  if (_sourceFile != NULL)  {
    ASSERT_EQ(0, fclose(_sourceFile));
  }
  _sourceFile = NULL;

  if (_vpm != NULL)  {
    VideoProcessingModule::Destroy(_vpm);
  }
  _vpm = NULL;
}

TEST_F(VideoProcessingModuleTest, HandleNullBuffer)
{
  VideoProcessingModule::FrameStats stats;
  ASSERT_EQ(0, _vpm->GetFrameStats(stats, _videoFrame));
  
  VideoFrame videoFrame;
  videoFrame.SetWidth(_width);
  videoFrame.SetHeight(_height);

  EXPECT_EQ(-3, _vpm->GetFrameStats(stats, NULL, _width, _height));
  EXPECT_EQ(-3, _vpm->GetFrameStats(stats, videoFrame));

  EXPECT_EQ(-1, _vpm->ColorEnhancement(NULL, _width, _height));
  EXPECT_EQ(-1, _vpm->ColorEnhancement(videoFrame));

  EXPECT_EQ(-1, _vpm->Deflickering(NULL, _width, _height, 0, stats));
  EXPECT_EQ(-1, _vpm->Deflickering(videoFrame, stats));

  EXPECT_EQ(-1, _vpm->Denoising(NULL, _width, _height));
  EXPECT_EQ(-1, _vpm->Denoising(videoFrame));

  EXPECT_EQ(-3, _vpm->BrightnessDetection(NULL, _width, _height, stats));
  EXPECT_EQ(-3, _vpm->BrightnessDetection(videoFrame, stats));

  EXPECT_EQ(VPM_PARAMETER_ERROR, _vpm->PreprocessFrame(NULL, NULL));
}

TEST_F(VideoProcessingModuleTest, HandleBadStats)
{
  VideoProcessingModule::FrameStats stats;

  ASSERT_EQ(_frameLength, fread(_videoFrame.Buffer(), 1, _frameLength,
                                _sourceFile));

  EXPECT_EQ(-1, _vpm->Deflickering(_videoFrame.Buffer(), _width, _height, 0,
                                   stats));
  EXPECT_EQ(-1, _vpm->Deflickering(_videoFrame, stats));

  EXPECT_EQ(-3, _vpm->BrightnessDetection(_videoFrame.Buffer(), _width,
                                          _height, stats));
  EXPECT_EQ(-3, _vpm->BrightnessDetection(_videoFrame, stats));
}

TEST_F(VideoProcessingModuleTest, HandleBadSize)
{
  VideoProcessingModule::FrameStats stats;
  ASSERT_EQ(0, _vpm->GetFrameStats(stats, _videoFrame));

  
  _videoFrame.SetWidth(0);
  EXPECT_EQ(-3, _vpm->GetFrameStats(stats, _videoFrame.Buffer(), 0, _height));
  EXPECT_EQ(-3, _vpm->GetFrameStats(stats, _videoFrame));

  EXPECT_EQ(-1, _vpm->ColorEnhancement(_videoFrame.Buffer(), 0, _height));
  EXPECT_EQ(-1, _vpm->ColorEnhancement(_videoFrame));

  EXPECT_EQ(-1, _vpm->Deflickering(_videoFrame.Buffer(), 0, _height, 0,
                                   stats));
  EXPECT_EQ(-1, _vpm->Deflickering(_videoFrame, stats));

  EXPECT_EQ(-1, _vpm->Denoising(_videoFrame.Buffer(), 0, _height));
  EXPECT_EQ(-1, _vpm->Denoising(_videoFrame));

  EXPECT_EQ(-3, _vpm->BrightnessDetection(_videoFrame.Buffer(), 0, _height,
                                          stats));
  EXPECT_EQ(-3, _vpm->BrightnessDetection(_videoFrame, stats));


  
  _videoFrame.SetWidth(_width);
  _videoFrame.SetHeight(0);
  EXPECT_EQ(-3, _vpm->GetFrameStats(stats, _videoFrame.Buffer(), _width, 0));
  EXPECT_EQ(-3, _vpm->GetFrameStats(stats, _videoFrame));

  EXPECT_EQ(-1, _vpm->ColorEnhancement(_videoFrame.Buffer(), _width, 0));
  EXPECT_EQ(-1, _vpm->ColorEnhancement(_videoFrame));

  EXPECT_EQ(-1, _vpm->Deflickering(_videoFrame.Buffer(), _width, 0, 0,
                                   stats));
  EXPECT_EQ(-1, _vpm->Deflickering(_videoFrame, stats));

  EXPECT_EQ(-1, _vpm->Denoising(_videoFrame.Buffer(), _width, 0));
  EXPECT_EQ(-1, _vpm->Denoising(_videoFrame));

  EXPECT_EQ(-3, _vpm->BrightnessDetection(_videoFrame.Buffer(), _width, 0,
                                          stats));
  EXPECT_EQ(-3, _vpm->BrightnessDetection(_videoFrame, stats));

  EXPECT_EQ(VPM_PARAMETER_ERROR, _vpm->SetTargetResolution(0,0,0));
  EXPECT_EQ(VPM_PARAMETER_ERROR, _vpm->SetMaxFrameRate(0));

  VideoFrame *outFrame = NULL;
  EXPECT_EQ(VPM_PARAMETER_ERROR, _vpm->PreprocessFrame(&_videoFrame,
                                                       &outFrame));
}

TEST_F(VideoProcessingModuleTest, IdenticalResultsAfterReset)
{
  VideoFrame videoFrame2;
  VideoProcessingModule::FrameStats stats;

  ASSERT_EQ(0, videoFrame2.VerifyAndAllocate(_frameLength));
  videoFrame2.SetWidth(_width);
  videoFrame2.SetHeight(_height);

  
  ASSERT_EQ(_frameLength, fread(_videoFrame.Buffer(), 1, _frameLength,
                                _sourceFile));
  ASSERT_EQ(0, _vpm->GetFrameStats(stats, _videoFrame));
  memcpy(videoFrame2.Buffer(), _videoFrame.Buffer(), _frameLength);
  ASSERT_EQ(0, _vpm->Deflickering(_videoFrame, stats));
  _vpm->Reset();
  
  ASSERT_EQ(0, _vpm->GetFrameStats(stats, videoFrame2));
  ASSERT_EQ(0, _vpm->Deflickering(videoFrame2, stats));
  EXPECT_EQ(0, memcmp(_videoFrame.Buffer(), videoFrame2.Buffer(),
                      _frameLength));

  ASSERT_EQ(_frameLength, fread(_videoFrame.Buffer(), 1, _frameLength,
                                _sourceFile));
  memcpy(videoFrame2.Buffer(), _videoFrame.Buffer(), _frameLength);
  ASSERT_GE(_vpm->Denoising(_videoFrame), 0);
  _vpm->Reset();
  ASSERT_GE(_vpm->Denoising(videoFrame2), 0);
  EXPECT_EQ(0, memcmp(_videoFrame.Buffer(), videoFrame2.Buffer(),
                      _frameLength));

  ASSERT_EQ(_frameLength, fread(_videoFrame.Buffer(), 1, _frameLength,
                                _sourceFile));
  ASSERT_EQ(0, _vpm->GetFrameStats(stats, _videoFrame));
  memcpy(videoFrame2.Buffer(), _videoFrame.Buffer(), _frameLength);
  ASSERT_EQ(0, _vpm->BrightnessDetection(_videoFrame, stats));
  _vpm->Reset();
  ASSERT_EQ(0, _vpm->BrightnessDetection(videoFrame2, stats));
  EXPECT_EQ(0, memcmp(_videoFrame.Buffer(), videoFrame2.Buffer(),
                      _frameLength));
}

TEST_F(VideoProcessingModuleTest, FrameStats)
{
  VideoProcessingModule::FrameStats stats;
  ASSERT_EQ(_frameLength, fread(_videoFrame.Buffer(), 1, _frameLength,
                                _sourceFile));

  EXPECT_FALSE(_vpm->ValidFrameStats(stats));
  EXPECT_EQ(0, _vpm->GetFrameStats(stats, _videoFrame));
  EXPECT_TRUE(_vpm->ValidFrameStats(stats));

  printf("\nFrameStats\n");
  printf("mean: %u\nnumPixels: %u\nsubSamplWidth: "
         "%u\nsumSamplHeight: %u\nsum: %u\n\n",
         static_cast<unsigned int>(stats.mean),
         static_cast<unsigned int>(stats.numPixels),
         static_cast<unsigned int>(stats.subSamplHeight),
         static_cast<unsigned int>(stats.subSamplWidth),
         static_cast<unsigned int>(stats.sum));

  _vpm->ClearFrameStats(stats);
  EXPECT_FALSE(_vpm->ValidFrameStats(stats));
}

TEST_F(VideoProcessingModuleTest, PreprocessorLogic)
{
  
  _vpm->EnableTemporalDecimation(false);
  ASSERT_EQ(VPM_OK, _vpm->SetMaxFrameRate(30));
  ASSERT_EQ(VPM_OK, _vpm->SetTargetResolution(100, 100, 15));
  
  _vpm->EnableTemporalDecimation(true);
  ASSERT_EQ(VPM_OK, _vpm->SetTargetResolution(100, 100, 30));
  
  _vpm->SetInputFrameResampleMode(kNoRescaling);
  ASSERT_EQ(VPM_OK, _vpm->SetTargetResolution(100, 100, 30));
  VideoFrame *outFrame = NULL;
  ASSERT_EQ(VPM_OK, _vpm->PreprocessFrame(&_videoFrame, &outFrame));
  
  ASSERT_TRUE(outFrame == NULL);
}

TEST_F(VideoProcessingModuleTest, Resampler)
{
  enum { NumRuns = 1 };

  WebRtc_Word64 minRuntime = 0;
  WebRtc_Word64 avgRuntime = 0;

  TickTime t0;
  TickTime t1;
  TickInterval accTicks;
  WebRtc_Word32 height = 288;
  WebRtc_Word32 width = 352;
  WebRtc_Word32 lengthSourceFrame = width*height*3/2;

  rewind(_sourceFile);
  ASSERT_TRUE(_sourceFile != NULL) <<
      "Cannot read input file \n";

  
  _vpm->EnableContentAnalysis(false);
  
  _vpm->EnableTemporalDecimation(false);

  
  VideoFrame sourceFrame;
  ASSERT_EQ(0, sourceFrame.VerifyAndAllocate(lengthSourceFrame));
  EXPECT_GT(fread(sourceFrame.Buffer(), 1, lengthSourceFrame, _sourceFile), 0u);
  ASSERT_EQ(0, sourceFrame.SetLength(lengthSourceFrame));
  sourceFrame.SetHeight(height);
  sourceFrame.SetWidth(width);

  for (WebRtc_UWord32 runIdx = 0; runIdx < NumRuns; runIdx++)
  {
    
    t0 = TickTime::Now();

    
    
    TestSize(sourceFrame, 100, 50, 3, 24.0, _vpm);
    TestSize(sourceFrame, 352/4, 288/4, 3, 25.2, _vpm);
    TestSize(sourceFrame, 352/2, 288/2, 3, 28.1, _vpm);
    TestSize(sourceFrame, 352, 288, 3, -1, _vpm);  
    TestSize(sourceFrame, 2*352, 2*288, 3, 32.2, _vpm);
    TestSize(sourceFrame, 400, 256, 3, 31.3, _vpm);
    TestSize(sourceFrame, 480, 640, 3, 32.15, _vpm);
    TestSize(sourceFrame, 960, 720, 3, 32.2, _vpm);
    TestSize(sourceFrame, 1280, 720, 3, 32.15, _vpm);
    
    TestSize(sourceFrame, 501, 333, 3, 32.05, _vpm);
    
    TestSize(sourceFrame, 281, 175, 3, 29.3, _vpm);

    
    t1 = TickTime::Now();
    accTicks += t1 - t0;

    if (accTicks.Microseconds() < minRuntime || runIdx == 0)  {
      minRuntime = accTicks.Microseconds();
    }
    avgRuntime += accTicks.Microseconds();
  }

  sourceFrame.Free();

  printf("\nAverage run time = %d us / frame\n",
         
         static_cast<int>(avgRuntime));
  printf("Min run time = %d us / frame\n\n",
         
         static_cast<int>(minRuntime));
}

void TestSize(const VideoFrame& source_frame, int target_width,
              int target_height, int mode, double expected_psnr,
              VideoProcessingModule* vpm) {
  int source_width = source_frame.Width();
  int source_height = source_frame.Height();
  VideoFrame* out_frame = NULL;

  ASSERT_EQ(VPM_OK, vpm->SetTargetResolution(target_width, target_height, 30));
  ASSERT_EQ(VPM_OK, vpm->PreprocessFrame(&source_frame, &out_frame));

  
  
  
  
  
  if (target_width != static_cast<int>(source_width) ||
      target_height != static_cast<int>(source_height))  {
    int target_half_width = (target_width + 1) >> 1;
    int target_half_height = (target_height + 1) >> 1;
    int required_size_resampled = target_width * target_height +
        2 * (target_half_width * target_half_height);
    ASSERT_EQ(required_size_resampled, static_cast<int>(out_frame->Length()));

    
    std::ostringstream filename;
    filename << webrtc::test::OutputPath() << "Resampler_"<< mode << "_" <<
        "from_" << source_width << "x" << source_height << "_to_" <<
        target_width << "x" << target_height << "_30Hz_P420.yuv";
    std::cout << "Watch " << filename.str() << " and verify that it is okay."
        << std::endl;
    FILE* stand_alone_file = fopen(filename.str().c_str(), "wb");
    if (fwrite(out_frame->Buffer(), 1,
               out_frame->Length(), stand_alone_file) != out_frame->Length()) {
      fprintf(stderr, "Failed to write frame for scaling to width/height: "
          " %d %d \n", target_width, target_height);
      return;
    }
    fclose(stand_alone_file);

    VideoFrame resampled_source_frame;
    resampled_source_frame.CopyFrame(*out_frame);

    
    ASSERT_EQ(VPM_OK, vpm->SetTargetResolution(source_width,
                                               source_height,
                                               30));
    ASSERT_EQ(VPM_OK, vpm->PreprocessFrame(&resampled_source_frame,
                                           &out_frame));

    
    std::ostringstream filename2;
    filename2 << webrtc::test::OutputPath() << "Resampler_"<< mode << "_" <<
          "from_" << target_width << "x" << target_height << "_to_" <<
          source_width << "x" << source_height << "_30Hz_P420.yuv";
    std::cout << "Watch " << filename2.str() << " and verify that it is okay."
                << std::endl;
    stand_alone_file = fopen(filename2.str().c_str(), "wb");
    if (fwrite(out_frame->Buffer(), 1,
               out_frame->Length(), stand_alone_file) != out_frame->Length()) {
      fprintf(stderr, "Failed to write frame for scaling to width/height "
          "%d %d \n", source_width, source_height);
      return;
    }
    fclose(stand_alone_file);

    
    double psnr = I420PSNR(source_frame.Buffer(), out_frame->Buffer(),
                           source_width, source_height);
    EXPECT_GT(psnr, expected_psnr);
    printf("PSNR: %f. PSNR is between source of size %d %d, and a modified "
        "source which is scaled down/up to: %d %d, and back to source size \n",
        psnr, source_width, source_height, target_width, target_height);

    resampled_source_frame.Free();
  }
}

}  
