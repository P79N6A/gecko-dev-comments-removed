









#include "../unit_test/unit_test.h"

#include <stdlib.h>  

#include <cstring>



#define BENCHMARK_ITERATIONS 1

libyuvTest::libyuvTest() : rotate_max_w_(128), rotate_max_h_(128),
    benchmark_iterations_(BENCHMARK_ITERATIONS), benchmark_width_(128),
    benchmark_height_(72) {
    const char* repeat = getenv("LIBYUV_REPEAT");
    if (repeat) {
      benchmark_iterations_ = atoi(repeat);  
      
      
      if (benchmark_iterations_ > 1) {
        benchmark_width_ = 1280;
        benchmark_height_ = 720;
      }
    }
    const char* width = getenv("LIBYUV_WIDTH");
    if (width) {
      benchmark_width_ = atoi(width);  
    }
    const char* height = getenv("LIBYUV_HEIGHT");
    if (height) {
      benchmark_height_ = atoi(height);  
    }
    benchmark_pixels_div256_ = static_cast<int>((
        static_cast<double>(Abs(benchmark_width_)) *
        static_cast<double>(Abs(benchmark_height_)) *
        static_cast<double>(benchmark_iterations_)  + 255.0) / 256.0);
    benchmark_pixels_div1280_ = static_cast<int>((
        static_cast<double>(Abs(benchmark_width_)) *
        static_cast<double>(Abs(benchmark_height_)) *
        static_cast<double>(benchmark_iterations_)  + 1279.0) / 1280.0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
