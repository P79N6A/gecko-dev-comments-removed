









#include "gflags/gflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/field_trial.h"
#include "webrtc/test/testsupport/fileutils.h"

DEFINE_string(force_fieldtrials, "",
    "Field trials control experimental feature code which can be forced. "
    "E.g. running with --force_fieldtrials=WebRTC-FooFeature/Enable/"
    " will assign the group Enable to field trial WebRTC-FooFeature.");

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  
  
  google::AllowCommandLineReparsing();
  google::ParseCommandLineFlags(&argc, &argv, false);

  webrtc::test::SetExecutablePath(argv[0]);
  webrtc::test::InitFieldTrialsFromString(FLAGS_force_fieldtrials);
  return RUN_ALL_TESTS();
}
