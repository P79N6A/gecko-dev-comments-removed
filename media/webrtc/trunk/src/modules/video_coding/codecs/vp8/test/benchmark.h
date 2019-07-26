









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_BENCHMARK_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_BENCHMARK_H_

#include "modules/video_coding/codecs/test_framework/benchmark.h"

class VP8Benchmark : public Benchmark
{
public:
    VP8Benchmark();
    VP8Benchmark(std::string name, std::string description);
    VP8Benchmark(std::string name, std::string description, std::string resultsFileName);

protected:
    virtual webrtc::VideoEncoder* GetNewEncoder();
    virtual webrtc::VideoDecoder* GetNewDecoder();
};

#endif 
