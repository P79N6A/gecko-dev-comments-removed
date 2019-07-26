









#include "gtest/gtest.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/system_wrappers/source/cpu_measurement_harness.h"
#include "webrtc/test/testsupport/fileutils.h"

using webrtc::CpuMeasurementHarness;
using webrtc::Trace;
using webrtc::kTraceWarning;
using webrtc::kTraceUtility;

class Logger : public webrtc::CpuTarget {
 public:
  Logger() {
    Trace::CreateTrace();
    std::string trace_file = webrtc::test::OutputPath() + "trace_unittest.txt";
    Trace::SetTraceFile(trace_file.c_str());
    Trace::SetLevelFilter(webrtc::kTraceAll);
  }
  virtual ~Logger() {
    Trace::ReturnTrace();
  }

  virtual bool DoWork() {
    
    
    
    WEBRTC_TRACE(kTraceWarning, kTraceUtility, 0, "Log line");
    return true;
  }
};



TEST(TraceTest, DISABLED_CpuUsage) {
  Logger logger;
  const int periodicity_ms = 1;
  const int iterations_per_period = 10;
  const int duration_ms = 1000;
  CpuMeasurementHarness* cpu_harness =
    CpuMeasurementHarness::Create(&logger, periodicity_ms,
                                  iterations_per_period, duration_ms);
  cpu_harness->Run();
  const int average_cpu = cpu_harness->AverageCpu();
  EXPECT_GE(5, average_cpu);
}
