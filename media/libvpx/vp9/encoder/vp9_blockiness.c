









#include "./vpx_config.h"
#include "./vp9_rtcd.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_convolve.h"
#include "vp9/common/vp9_filter.h"
#include "vpx/vpx_integer.h"
#include "vpx_ports/mem.h"

static int horizontal_filter(const uint8_t *s) {
  return (s[1] - s[-2]) * 2 + (s[-1] - s[0]) * 6;
}

static int vertical_filter(const uint8_t *s, int p) {
  return (s[p] - s[-2 * p]) * 2 + (s[-p] - s[0]) * 6;
}

static int variance(int sum, int sum_squared, int size) {
  return sum_squared / size - (sum / size) * (sum / size);
}

























int blockiness_vertical(const uint8_t *s, int sp, const uint8_t *r, int rp,
                        int size) {
  int s_blockiness = 0;
  int r_blockiness = 0;
  int sum_0 = 0;
  int sum_sq_0 = 0;
  int sum_1 = 0;
  int sum_sq_1 = 0;
  int i;
  int var_0;
  int var_1;
  for (i = 0; i < size; ++i, s += sp, r += rp) {
    s_blockiness += horizontal_filter(s);
    r_blockiness += horizontal_filter(r);
    sum_0 += s[0];
    sum_sq_0 += s[0]*s[0];
    sum_1 += s[-1];
    sum_sq_1 += s[-1]*s[-1];
  }
  var_0 = variance(sum_0, sum_sq_0, size);
  var_1 = variance(sum_1, sum_sq_1, size);
  r_blockiness = abs(r_blockiness);
  s_blockiness = abs(s_blockiness);

  if (r_blockiness > s_blockiness)
    return (r_blockiness - s_blockiness) / (1 + var_0 + var_1);
  else
    return 0;
}



int blockiness_horizontal(const uint8_t *s, int sp, const uint8_t *r, int rp,
                          int size) {
  int s_blockiness = 0;
  int r_blockiness = 0;
  int sum_0 = 0;
  int sum_sq_0 = 0;
  int sum_1 = 0;
  int sum_sq_1 = 0;
  int i;
  int var_0;
  int var_1;
  for (i = 0; i < size; ++i, ++s, ++r) {
    s_blockiness += vertical_filter(s, sp);
    r_blockiness += vertical_filter(r, rp);
    sum_0 += s[0];
    sum_sq_0 += s[0] * s[0];
    sum_1 += s[-sp];
    sum_sq_1 += s[-sp] * s[-sp];
  }
  var_0 = variance(sum_0, sum_sq_0, size);
  var_1 = variance(sum_1, sum_sq_1, size);
  r_blockiness = abs(r_blockiness);
  s_blockiness = abs(s_blockiness);

  if (r_blockiness > s_blockiness)
    return (r_blockiness - s_blockiness) / (1 + var_0 + var_1);
  else
    return 0;
}



double vp9_get_blockiness(const unsigned char *img1, int img1_pitch,
                          const unsigned char *img2, int img2_pitch,
                          int width, int height ) {
  double blockiness = 0;
  int i, j;
  vp9_clear_system_state();
  for (i = 0; i < height; i += 4, img1 += img1_pitch * 4,
       img2 += img2_pitch * 4) {
    for (j = 0; j < width; j += 4) {
      if (i > 0 && i < height && j > 0 && j < width) {
        blockiness += blockiness_vertical(img1 + j, img1_pitch,
                                          img2 + j, img2_pitch, 4);
        blockiness += blockiness_horizontal(img1 + j, img1_pitch,
                                            img2 + j, img2_pitch, 4);
      }
    }
  }
  blockiness /= width * height / 16;
  return blockiness;
}
