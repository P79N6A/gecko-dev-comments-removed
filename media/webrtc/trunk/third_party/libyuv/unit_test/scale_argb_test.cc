









#include <stdlib.h>
#include <time.h>

#include "libyuv/cpu_id.h"
#include "libyuv/scale_argb.h"
#include "../unit_test/unit_test.h"

namespace libyuv {

static int ARGBTestFilter(int src_width, int src_height,
                          int dst_width, int dst_height,
                          FilterMode f, int benchmark_iterations) {
  const int b = 128;
  int src_argb_plane_size = (src_width + b * 2) * (src_height + b * 2) * 4;
  int src_stride_argb = (b * 2 + src_width) * 4;

  align_buffer_16(src_argb, src_argb_plane_size)
  memset(src_argb, 1, src_argb_plane_size);

  int dst_argb_plane_size = (dst_width + b * 2) * (dst_height + b * 2) * 4;
  int dst_stride_argb = (b * 2 + dst_width) * 4;

  srandom(time(NULL));

  int i, j;
  for (i = b; i < (src_height + b); ++i) {
    for (j = b; j < (src_width + b) * 4; ++j) {
      src_argb[(i * src_stride_argb) + j] = (random() & 0xff);
    }
  }

  align_buffer_16(dst_argb_c, dst_argb_plane_size)
  align_buffer_16(dst_argb_opt, dst_argb_plane_size)
  memset(dst_argb_c, 2, dst_argb_plane_size);
  memset(dst_argb_opt, 3, dst_argb_plane_size);

  
  MaskCpuFlags(0);  
  ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
            src_width, src_height,
            dst_argb_c + (dst_stride_argb * b) + b * 4, dst_stride_argb,
            dst_width, dst_height, f);
  MaskCpuFlags(-1);  
  ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
            src_width, src_height,
            dst_argb_opt + (dst_stride_argb * b) + b * 4, dst_stride_argb,
            dst_width, dst_height, f);

  MaskCpuFlags(0);  
  double c_time = get_time();
  for (i = 0; i < benchmark_iterations; ++i) {
    ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
              src_width, src_height,
              dst_argb_c + (dst_stride_argb * b) + b * 4, dst_stride_argb,
              dst_width, dst_height, f);
  }
  c_time = (get_time() - c_time) / benchmark_iterations;

  MaskCpuFlags(-1);  
  double opt_time = get_time();
  for (i = 0; i < benchmark_iterations; ++i) {
    ARGBScale(src_argb + (src_stride_argb * b) + b * 4, src_stride_argb,
              src_width, src_height,
              dst_argb_opt + (dst_stride_argb * b) + b * 4, dst_stride_argb,
              dst_width, dst_height, f);
  }
  opt_time = (get_time() - opt_time) / benchmark_iterations;

  
  printf("filter %d - %8d us C - %8d us OPT\n",
         f, static_cast<int>(c_time*1e6), static_cast<int>(opt_time*1e6));

  
  
  
  
  int max_diff = 0;
  for (i = b; i < (dst_height + b); ++i) {
    for (j = b * 4; j < (dst_width + b) * 4; ++j) {
      int abs_diff = abs(dst_argb_c[(i * dst_stride_argb) + j] -
                         dst_argb_opt[(i * dst_stride_argb) + j]);
      if (abs_diff > max_diff) {
        max_diff = abs_diff;
      }
    }
  }

  free_aligned_buffer_16(dst_argb_c)
  free_aligned_buffer_16(dst_argb_opt)
  free_aligned_buffer_16(src_argb)
  return max_diff;
}

TEST_F(libyuvTest, ARGBScaleDownBy2) {
  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width / 2;
  const int dst_height = src_height / 2;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleDownBy4) {
  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width / 4;
  const int dst_height = src_height / 4;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleDownBy5) {
  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width / 5;
  const int dst_height = src_height / 5;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleDownBy8) {
  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width / 8;
  const int dst_height = src_height / 8;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleDownBy16) {
  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width / 16;
  const int dst_height = src_height / 16;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleDownBy34) {
  const int src_width = 1280;
  const int src_height = 720;
  const int dst_width = src_width * 3 / 4;
  const int dst_height = src_height * 3 / 4;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleDownBy38) {
  int src_width = 1280;
  int src_height = 720;
  int dst_width = src_width * 3 / 8;
  int dst_height = src_height * 3 / 8;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleTo1366) {
  int src_width = 1280;
  int src_height = 720;
  int dst_width = 1366;
  int dst_height = 768;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

TEST_F(libyuvTest, ARGBScaleTo4074) {
  int src_width = 2880 * 2;
  int src_height = 1800;
  int dst_width = 4074;
  int dst_height = 1272;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}


TEST_F(libyuvTest, ARGBScaleTo853) {
  int src_width = 1280;
  int src_height = 720;
  int dst_width = 853;
  int dst_height = 480;

  for (int f = 0; f < 2; ++f) {
    int max_diff = ARGBTestFilter(src_width, src_height,
                                  dst_width, dst_height,
                                  static_cast<FilterMode>(f),
                                  benchmark_iterations_);
    EXPECT_LE(max_diff, 1);
  }
}

}  
