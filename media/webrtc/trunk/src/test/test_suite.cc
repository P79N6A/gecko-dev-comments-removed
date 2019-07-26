









#include "test/test_suite.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace webrtc {
namespace test {
TestSuite::TestSuite(int argc, char** argv) {
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
  
}

void TestSuite::Shutdown() {
}
}  
}  
