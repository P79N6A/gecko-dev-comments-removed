









#include "./vp9_rtcd.h"

void vp9_lpf_horizontal_8_dual_neon(uint8_t *s, int p ,
                                    const uint8_t *blimit0,
                                    const uint8_t *limit0,
                                    const uint8_t *thresh0,
                                    const uint8_t *blimit1,
                                    const uint8_t *limit1,
                                    const uint8_t *thresh1) {
  vp9_lpf_horizontal_8(s, p, blimit0, limit0, thresh0, 1);
  vp9_lpf_horizontal_8(s + 8, p, blimit1, limit1, thresh1, 1);
}

void vp9_lpf_vertical_4_dual_neon(uint8_t *s, int p,
                                  const uint8_t *blimit0,
                                  const uint8_t *limit0,
                                  const uint8_t *thresh0,
                                  const uint8_t *blimit1,
                                  const uint8_t *limit1,
                                  const uint8_t *thresh1) {
  vp9_lpf_vertical_4_neon(s, p, blimit0, limit0, thresh0, 1);
  vp9_lpf_vertical_4_neon(s + 8 * p, p, blimit1, limit1, thresh1, 1);
}

void vp9_lpf_vertical_8_dual_neon(uint8_t *s, int p,
                                  const uint8_t *blimit0,
                                  const uint8_t *limit0,
                                  const uint8_t *thresh0,
                                  const uint8_t *blimit1,
                                  const uint8_t *limit1,
                                  const uint8_t *thresh1) {
  vp9_lpf_vertical_8_neon(s, p, blimit0, limit0, thresh0, 1);
  vp9_lpf_vertical_8_neon(s + 8 * p, p, blimit1, limit1, thresh1, 1);
}

void vp9_lpf_vertical_16_dual_neon(uint8_t *s, int p,
                                   const uint8_t *blimit,
                                   const uint8_t *limit,
                                   const uint8_t *thresh) {
  vp9_lpf_vertical_16_neon(s, p, blimit, limit, thresh);
  vp9_lpf_vertical_16_neon(s + 8 * p, p, blimit, limit, thresh);
}
