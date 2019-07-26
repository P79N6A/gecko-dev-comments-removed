









#include <stdio.h>

#include <iostream>

#include "gflags/gflags.h"
#include "webrtc/modules/audio_coding/neteq4/tools/neteq_performance_test.h"
#include "webrtc/typedefs.h"


static bool ValidateRuntime(const char* flagname, int value) {
  if (value > 0)  
    return true;
  printf("Invalid value for --%s: %d\n", flagname, static_cast<int>(value));
  return false;
}
static bool ValidateLossrate(const char* flagname, int value) {
  if (value >= 0)  
    return true;
  printf("Invalid value for --%s: %d\n", flagname, static_cast<int>(value));
  return false;
}
static bool ValidateDriftfactor(const char* flagname, double value) {
  if (value >= 0.0 && value < 1.0)  
    return true;
  printf("Invalid value for --%s: %f\n", flagname, value);
  return false;
}


DEFINE_int32(runtime_ms, 10000, "Simulated runtime in ms.");
static const bool runtime_ms_dummy =
    google::RegisterFlagValidator(&FLAGS_runtime_ms, &ValidateRuntime);
DEFINE_int32(lossrate, 10,
             "Packet lossrate; drop every N packets.");
static const bool lossrate_dummy =
    google::RegisterFlagValidator(&FLAGS_lossrate, &ValidateLossrate);
DEFINE_double(drift, 0.1,
             "Clockdrift factor.");
static const bool drift_dummy =
    google::RegisterFlagValidator(&FLAGS_drift, &ValidateDriftfactor);

int main(int argc, char* argv[]) {
  std::string program_name = argv[0];
  std::string usage = "Tool for measuring the speed of NetEq.\n"
      "Usage: " + program_name + " [options]\n\n"
      "  --runtime_ms=N         runtime in ms; default is 10000 ms\n"
      "  --lossrate=N           drop every N packets; default is 10\n"
      "  --drift=F              clockdrift factor between 0.0 and 1.0; "
      "default is 0.1\n";
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (argc != 1) {
    
    std::cout << google::ProgramUsage();
    return 0;
  }

  int64_t result =
      webrtc::test::NetEqPerformanceTest::Run(FLAGS_runtime_ms, FLAGS_lossrate,
                                              FLAGS_drift);
  if (result <= 0) {
    std::cout << "There was an error" << std::endl;
    return -1;
  }

  std::cout << "Simulation done" << std::endl;
  std::cout << "Runtime = " << result << " ms" << std::endl;
  return 0;
}
