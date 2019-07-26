









#include "unit_test/unit_test.h"

#include <cstring>

libyuvTest::libyuvTest() : rotate_max_w_(128), rotate_max_h_(128),
    benchmark_iterations_(1000), benchmark_width_(1280),
    benchmark_height_(720) {
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
