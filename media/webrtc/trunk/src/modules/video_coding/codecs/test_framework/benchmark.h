









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAWEWORK_BENCHMARK_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAWEWORK_BENCHMARK_H_

#include "normal_async_test.h"

class VideoSource;

class Benchmark : public NormalAsyncTest
{
public:
    Benchmark();
    virtual void Perform();

protected:
    Benchmark(std::string name, std::string description);
    Benchmark(std::string name, std::string description, std::string resultsFileName, std::string codecName);
    virtual webrtc::VideoEncoder* GetNewEncoder() = 0;
    virtual webrtc::VideoDecoder* GetNewDecoder() = 0;
    virtual void PerformNormalTest();
    virtual void CodecSpecific_InitBitrate();
    static const char* GetMagicStr() { return "#!benchmark1.0"; }

    const VideoSource* _target;
    std::string        _resultsFileName;
    std::ofstream      _results;
    std::string        _codecName;
};

#endif 

