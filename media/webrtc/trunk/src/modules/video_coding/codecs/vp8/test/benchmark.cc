









#include "benchmark.h"
#include "testsupport/fileutils.h"
#include "vp8.h"

using namespace webrtc;

VP8Benchmark::VP8Benchmark()
    : Benchmark("VP8Benchmark", "VP8 benchmark over a range of test cases",
                webrtc::test::OutputPath() + "VP8Benchmark.txt", "VP8") {
}

VP8Benchmark::VP8Benchmark(std::string name, std::string description)
    : Benchmark(name, description,
                webrtc::test::OutputPath() + "VP8Benchmark.txt",
                "VP8") {
}

VP8Benchmark::VP8Benchmark(std::string name, std::string description,
                           std::string resultsFileName)
    : Benchmark(name, description, resultsFileName, "VP8") {
}

VideoEncoder* VP8Benchmark::GetNewEncoder() {
    return VP8Encoder::Create();
}

VideoDecoder* VP8Benchmark::GetNewDecoder() {
    return VP8Decoder::Create();
}
