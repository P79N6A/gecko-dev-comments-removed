









#include <cstdio>
#include <cstdlib>

#include "modules/video_processing/main/interface/video_processing.h"
#include "modules/video_processing/main/test/unit_test/unit_test.h"
#include "system_wrappers/interface/tick_util.h"
#include "testsupport/fileutils.h"

namespace webrtc {

TEST_F(VideoProcessingModuleTest, Deflickering)
{
    enum { NumRuns = 30 };
    WebRtc_UWord32 frameNum = 0;
    const WebRtc_UWord32 frameRate = 15;

    WebRtc_Word64 minRuntime = 0;
    WebRtc_Word64 avgRuntime = 0;

    
    fclose(_sourceFile);
    const std::string input_file =
        webrtc::test::ResourcePath("deflicker_before_cif_short", "yuv");
    _sourceFile  = fopen(input_file.c_str(), "rb");
    ASSERT_TRUE(_sourceFile != NULL) <<
        "Cannot read input file: " << input_file << "\n";

    const std::string output_file =
        webrtc::test::OutputPath() + "deflicker_output_cif_short.yuv";
    FILE* deflickerFile = fopen(output_file.c_str(), "wb");
    ASSERT_TRUE(deflickerFile != NULL) <<
        "Could not open output file: " << output_file << "\n";

    printf("\nRun time [us / frame]:\n");
    for (WebRtc_UWord32 runIdx = 0; runIdx < NumRuns; runIdx++)
    {
        TickTime t0;
        TickTime t1;
        TickInterval accTicks;
        WebRtc_UWord32 timeStamp = 1;

        frameNum = 0;
        while (fread(_videoFrame.Buffer(), 1, _frameLength, _sourceFile) == _frameLength)
        {
            frameNum++;
            _videoFrame.SetTimeStamp(timeStamp);

            t0 = TickTime::Now();
            VideoProcessingModule::FrameStats stats;
            ASSERT_EQ(0, _vpm->GetFrameStats(stats, _videoFrame));
            ASSERT_EQ(0, _vpm->Deflickering(_videoFrame, stats));
            t1 = TickTime::Now();
            accTicks += t1 - t0;

            if (runIdx == 0)
            {
              if (fwrite(_videoFrame.Buffer(), 1, _frameLength,
                         deflickerFile) !=  _frameLength) {
                return;
              }
            }
            timeStamp += (90000 / frameRate);
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
    ASSERT_EQ(0, fclose(deflickerFile));
    

    printf("\nAverage run time = %d us / frame\n",
        static_cast<int>(avgRuntime / frameNum / NumRuns));
    printf("Min run time = %d us / frame\n\n",
        static_cast<int>(minRuntime / frameNum));
}

}  
