









#include "webrtc/test/test_suite.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/trace_to_stderr.h"

namespace webrtc {
namespace test {

TestSuite::TestSuite(int argc, char** argv)
    : trace_to_stderr_(NULL) {
  SetExecutablePath(argv[0]);
  testing::InitGoogleMock(&argc, argv);  
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
  
  trace_to_stderr_.reset(new TraceToStderr);
}

void TestSuite::Shutdown() {
}

}  
}  
