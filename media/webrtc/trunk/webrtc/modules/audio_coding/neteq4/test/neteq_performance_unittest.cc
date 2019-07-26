









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/tools/neteq_performance_test.h"
#include "webrtc/test/testsupport/perf_test.h"
#include "webrtc/typedefs.h"



TEST(NetEqPerformanceTest, Run) {
  const int kSimulationTimeMs = 10000000;
  const int kLossPeriod = 10;  
  const double kDriftFactor = 0.1;
  int64_t runtime = webrtc::test::NetEqPerformanceTest::Run(
      kSimulationTimeMs, kLossPeriod, kDriftFactor);
  ASSERT_GT(runtime, 0);
  webrtc::test::PrintResult(
      "neteq_performance", "", "10_pl_10_drift", runtime, "ms", true);
}




TEST(NetEqPerformanceTest, RunClean) {
  const int kSimulationTimeMs = 10000000;
  const int kLossPeriod = 0;  
  const double kDriftFactor = 0.0;  
  int64_t runtime = webrtc::test::NetEqPerformanceTest::Run(
      kSimulationTimeMs, kLossPeriod, kDriftFactor);
  ASSERT_GT(runtime, 0);
  webrtc::test::PrintResult(
      "neteq_performance", "", "0_pl_0_drift", runtime, "ms", true);
}
