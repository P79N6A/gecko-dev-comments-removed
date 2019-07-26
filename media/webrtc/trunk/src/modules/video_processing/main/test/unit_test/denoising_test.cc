









#include <cstdio>
#include <cstdlib>

#include "modules/video_processing/main/interface/video_processing.h"
#include "modules/video_processing/main/test/unit_test/unit_test.h"
#include "system_wrappers/interface/tick_util.h"
#include "testsupport/fileutils.h"

namespace webrtc {

TEST_F(VideoProcessingModuleTest, Denoising)
{
    enum { NumRuns = 10 };
    WebRtc_UWord32 frameNum = 0;

    WebRtc_Word64 minRuntime = 0;
    WebRtc_Word64 avgRuntime = 0;

    const std::string denoise_filename =
        webrtc::test::OutputPath() + "denoise_testfile.yuv";
    FILE* denoiseFile = fopen(denoise_filename.c_str(), "wb");
    ASSERT_TRUE(denoiseFile != NULL) <<
        "Could not open output file: " << denoise_filename << "\n";

    const std::string noise_filename =
        webrtc::test::OutputPath() + "noise_testfile.yuv";
    FILE* noiseFile = fopen(noise_filename.c_str(), "wb");
    ASSERT_TRUE(noiseFile != NULL) <<
        "Could not open noisy file: " << noise_filename << "\n";

    printf("\nRun time [us / frame]:\n");
    for (WebRtc_UWord32 runIdx = 0; runIdx < NumRuns; runIdx++)
    {
        TickTime t0;
        TickTime t1;
        TickInterval accTicks;
        WebRtc_Word32 modifiedPixels = 0;

        frameNum = 0;
        while (fread(_videoFrame.Buffer(), 1, _frameLength, _sourceFile) == _frameLength)
        {
            frameNum++;
            WebRtc_UWord8* sourceBuffer = _videoFrame.Buffer();

            
            
            

            
            
            for (WebRtc_UWord32 ir = 0; ir < _height; ir++)
            {
                WebRtc_UWord32 ik = ir * _width;
                for (WebRtc_UWord32 ic = 0; ic < _width; ic++)
                {
                    WebRtc_UWord8 r = rand() % 16;
                    r -= 8;
                    if (ir < _height / 4)
                        r = 0;
                    if (ir >= 3 * _height / 4)
                        r = 0;
                    if (ic < _width / 4)
                        r = 0;
                    if (ic >= 3 * _width / 4)
                        r = 0;

                    











                    sourceBuffer[ik + ic] += r;
                }
            }

            if (runIdx == 0)
            {
              if (fwrite(_videoFrame.Buffer(), 1, _frameLength,
                         noiseFile) !=  _frameLength) {
                return;
              }
            }

            t0 = TickTime::Now();
            ASSERT_GE(modifiedPixels = _vpm->Denoising(_videoFrame), 0);
            t1 = TickTime::Now();
            accTicks += t1 - t0;

            if (runIdx == 0)
            {
              if (fwrite(_videoFrame.Buffer(), 1, _frameLength,
                         denoiseFile) !=  _frameLength) {
                return;
              }
            }
        }
        ASSERT_NE(0, feof(_sourceFile)) << "Error reading source file";

        printf("%u\n", static_cast<int>(accTicks.Microseconds() / frameNum));
        if (accTicks.Microseconds() < minRuntime || runIdx == 0)
        {
            minRuntime = accTicks.Microseconds();
        }
        avgRuntime += accTicks.Microseconds();

        rewind(_sourceFile);
    }
    ASSERT_EQ(0, fclose(denoiseFile));
    ASSERT_EQ(0, fclose(noiseFile));
    printf("\nAverage run time = %d us / frame\n",
        static_cast<int>(avgRuntime / frameNum / NumRuns));
    printf("Min run time = %d us / frame\n\n",
        static_cast<int>(minRuntime / frameNum));
}

}  
