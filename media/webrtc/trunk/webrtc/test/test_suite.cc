









#include "webrtc/test/test_suite.h"

#include "gflags/gflags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/trace_to_stderr.h"
#include "webrtc/test/field_trial.h"

DEFINE_bool(logs, false, "print logs to stderr");

DEFINE_string(force_fieldtrials, "",
    "Field trials control experimental feature code which can be forced. "
    "E.g. running with --force_fieldtrials=WebRTC-FooFeature/Enable/"
    " will assign the group Enable to field trial WebRTC-FooFeature.");

namespace webrtc {
namespace test {

TestSuite::TestSuite(int argc, char** argv) {
  SetExecutablePath(argv[0]);
  testing::InitGoogleMock(&argc, argv);  
  
  
  google::AllowCommandLineReparsing();
  google::ParseCommandLineFlags(&argc, &argv, true);

  webrtc::test::InitFieldTrialsFromString(FLAGS_force_fieldtrials);
}

TestSuite::~TestSuite() {
}

int TestSuite::Run() {
  Initialize();
  int result = RUN_ALL_TESTS();
  Shutdown();
  return result;
}

void TestSuite::Initialize() {
  if (FLAGS_logs)
    trace_to_stderr_.reset(new TraceToStderr);
}

void TestSuite::Shutdown() {
}

}  
}  
