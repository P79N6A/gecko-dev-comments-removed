









#include "test/test_suite.h"

int main(int argc, char** argv) {
  webrtc::test::TestSuite test_suite(argc, argv);
  return test_suite.Run();
}
