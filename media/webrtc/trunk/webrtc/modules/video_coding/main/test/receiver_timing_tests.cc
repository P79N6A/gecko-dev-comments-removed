









#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/timing.h"
#include "webrtc/modules/video_coding/main/test/receiver_tests.h"
#include "webrtc/modules/video_coding/main/test/test_macros.h"
#include "webrtc/modules/video_coding/main/test/test_util.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"

using namespace webrtc;

float vcmFloatMax(float a, float b)
{
    return a > b ? a : b;
}

float vcmFloatMin(float a, float b)
{
    return a < b ? a : b;
}

double const pi = 4*std::atan(1.0);

class GaussDist
{
public:
    static float RandValue(float m, float stdDev) 
    {
        float r1 = static_cast<float>((std::rand() + 1.0)/(RAND_MAX + 1.0)); 
        float r2 = static_cast<float>((std::rand() + 1.0)/(RAND_MAX + 1.0));
        return m + stdDev * static_cast<float>(std::sqrt(-2*std::log(r1))*std::cos(2*pi*r2));
    }
};

int ReceiverTimingTests(CmdArgs& args)
{
    
    Trace::CreateTrace();
    Trace::SetTraceFile((test::OutputPath() + "receiverTestTrace.txt").c_str());
    Trace::SetLevelFilter(webrtc::kTraceAll);

    
    srand(0);

    Clock* clock = Clock::GetRealTimeClock();
    VCMTiming timing(clock);
    float clockInMs = 0.0;
    uint32_t waitTime = 0;
    uint32_t jitterDelayMs = 0;
    uint32_t maxDecodeTimeMs = 0;
    uint32_t timeStamp = 0;

    timing.Reset(static_cast<int64_t>(clockInMs + 0.5));

    timing.UpdateCurrentDelay(timeStamp);

    timing.Reset(static_cast<int64_t>(clockInMs + 0.5));

    timing.IncomingTimestamp(timeStamp, static_cast<int64_t>(clockInMs + 0.5));
    jitterDelayMs = 20;
    timing.SetRequiredDelay(jitterDelayMs);
    timing.UpdateCurrentDelay(timeStamp);
    waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5)),
        static_cast<int64_t>(clockInMs + 0.5));
    
    
    TEST(waitTime == jitterDelayMs);

    jitterDelayMs += VCMTiming::kDelayMaxChangeMsPerS + 10;
    timeStamp += 90000;
    clockInMs += 1000.0f;
    timing.SetRequiredDelay(jitterDelayMs);
    timing.UpdateCurrentDelay(timeStamp);
    waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5)),
        static_cast<int64_t>(clockInMs + 0.5));
    
    
    TEST(waitTime == jitterDelayMs - 10);

    timeStamp += 90000;
    clockInMs += 1000.0;
    timing.UpdateCurrentDelay(timeStamp);
    waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5)),
        static_cast<int64_t>(clockInMs + 0.5));
    TEST(waitTime == jitterDelayMs);

    
    for (int i=0; i < 300; i++)
    {
        clockInMs += 1000.0f/30.0f;
        timeStamp += 3000;
        timing.IncomingTimestamp(timeStamp, static_cast<int64_t>(clockInMs + 0.5));
    }
    timing.UpdateCurrentDelay(timeStamp);
    waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5)),
        static_cast<int64_t>(clockInMs + 0.5));
    TEST(waitTime == jitterDelayMs);

    
    for (int i=0; i < 10; i++)
    {
        int64_t startTimeMs = static_cast<int64_t>(clockInMs + 0.5);
        clockInMs += 10.0f;
        timing.StopDecodeTimer(timeStamp, startTimeMs, static_cast<int64_t>(clockInMs + 0.5));
        timeStamp += 3000;
        clockInMs += 1000.0f/30.0f - 10.0f;
        timing.IncomingTimestamp(timeStamp, static_cast<int64_t>(clockInMs + 0.5));
    }
    maxDecodeTimeMs = 10;
    timing.SetRequiredDelay(jitterDelayMs);
    clockInMs += 1000.0f;
    timeStamp += 90000;
    timing.UpdateCurrentDelay(timeStamp);
    waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5)),
        static_cast<int64_t>(clockInMs + 0.5));
    TEST(waitTime == jitterDelayMs);

    uint32_t totalDelay1 = timing.TargetVideoDelay();
    uint32_t minTotalDelayMs = 200;
    timing.SetMinimumTotalDelay(minTotalDelayMs);
    clockInMs += 5000.0f;
    timeStamp += 5*90000;
    timing.UpdateCurrentDelay(timeStamp);
    waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5)),
        static_cast<int64_t>(clockInMs + 0.5));
    uint32_t totalDelay2 = timing.TargetVideoDelay();
    
    TEST(waitTime == minTotalDelayMs - maxDecodeTimeMs - 10);
    
    
    TEST(totalDelay1 == totalDelay2);

    
    timing.SetMinimumTotalDelay(0);
    clockInMs += 5000.0f;
    timeStamp += 5*90000;
    timing.UpdateCurrentDelay(timeStamp);

    
    clockInMs += 1000.0f/30.0f;
    timeStamp += static_cast<uint32_t>(2.1*90000 + 0.5);
    int64_t ret = timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5));
    TEST(ret == -1);
    timing.Reset();

    
    
    
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,  "Stochastic test 1");

    jitterDelayMs = 60;
    maxDecodeTimeMs = 10;

    timeStamp = static_cast<uint32_t>(-10000); 
    clockInMs = 10000.0f;
    timing.Reset(static_cast<int64_t>(clockInMs + 0.5));

    float noise = 0.0f;
    for (int i=0; i < 1400; i++)
    {
        if (i == 400)
        {
            jitterDelayMs = 30;
        }
        else if (i == 700)
        {
            jitterDelayMs = 100;
        }
        else if (i == 1000)
        {
            minTotalDelayMs = 200;
            timing.SetMinimumTotalDelay(minTotalDelayMs);
        }
        else if (i == 1200)
        {
            minTotalDelayMs = 0;
            timing.SetMinimumTotalDelay(minTotalDelayMs);
        }
        int64_t startTimeMs = static_cast<int64_t>(clockInMs + 0.5);
        noise = vcmFloatMin(vcmFloatMax(GaussDist::RandValue(0, 2), -10.0f), 30.0f);
        clockInMs += 10.0f;
        timing.StopDecodeTimer(timeStamp, startTimeMs, static_cast<int64_t>(clockInMs + noise + 0.5));
        timeStamp += 3000;
        clockInMs += 1000.0f/30.0f - 10.0f;
        noise = vcmFloatMin(vcmFloatMax(GaussDist::RandValue(0, 8), -15.0f), 15.0f);
        timing.IncomingTimestamp(timeStamp, static_cast<int64_t>(clockInMs + noise + 0.5));
        timing.SetRequiredDelay(jitterDelayMs);
        timing.UpdateCurrentDelay(timeStamp);
        waitTime = timing.MaxWaitingTime(timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5)),
            static_cast<int64_t>(clockInMs + 0.5));

        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,  "timeStamp=%u clock=%u maxWaitTime=%u", timeStamp,
            static_cast<uint32_t>(clockInMs + 0.5), waitTime);

        int64_t renderTimeMs = timing.RenderTimeMs(timeStamp, static_cast<int64_t>(clockInMs + 0.5));

        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,
                   "timeStamp=%u renderTime=%u",
                   timeStamp,
                   MaskWord64ToUWord32(renderTimeMs));
    }
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,  "End Stochastic test 1");

    printf("\nVCM Timing Test: \n\n%i tests completed\n", vcmMacrosTests);
    if (vcmMacrosErrors > 0)
    {
        printf("%i FAILED\n\n", vcmMacrosErrors);
    }
    else
    {
        printf("ALL PASSED\n\n");
    }

    Trace::ReturnTrace();
    return 0;
}
