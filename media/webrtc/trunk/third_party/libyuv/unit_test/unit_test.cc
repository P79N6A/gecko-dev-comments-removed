









#include <cstring>
#include "unit_test.h"

libyuvTest::libyuvTest() :
  _rotate_max_w(128),
  _rotate_max_h(128),
  _benchmark_iterations(1000),
  _benchmark_width(1280),
  _benchmark_height(720) {
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
