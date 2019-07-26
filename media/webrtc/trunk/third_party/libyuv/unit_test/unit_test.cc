









#include "../unit_test/unit_test.h"

#include <stdlib.h>  

#include <cstring>



#define BENCHMARK_ITERATIONS 1

libyuvTest::libyuvTest() : rotate_max_w_(128), rotate_max_h_(128),
    benchmark_iterations_(BENCHMARK_ITERATIONS), benchmark_width_(1280),
    benchmark_height_(720) {
    const char* repeat = getenv("LIBYUV_REPEAT");
    if (repeat) {
      benchmark_iterations_ = atoi(repeat);  
    }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
