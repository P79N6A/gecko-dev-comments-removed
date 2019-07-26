









#include "webrtc/test/testsupport/perf_test.h"

#include <string>

#include "gtest/gtest.h"

namespace webrtc {
namespace test {

TEST(PerfTest, AppendResult) {
  std::string expected = "RESULT measurementmodifier: trace= 42 units\n";
  std::string output;
  AppendResult(output, "measurement", "modifier", "trace", 42, "units", false);
  EXPECT_EQ(expected, output);
  std::cout << output;

  expected += "*RESULT foobar: baz= 7 widgets\n";
  AppendResult(output, "foo", "bar", "baz", 7, "widgets", true);
  EXPECT_EQ(expected, output);
  std::cout << output;
}

}  
}  
