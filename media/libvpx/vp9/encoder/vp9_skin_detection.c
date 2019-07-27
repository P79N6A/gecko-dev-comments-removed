









#include <limits.h>
#include <math.h>

#include "vp9/common/vp9_blockd.h"
#include "vp9/encoder/vp9_encoder.h"
#include "vp9/encoder/vp9_skin_detection.h"


static const int skin_mean[2] = {7463, 9614};                 
static const int skin_inv_cov[4] = {4107, 1663, 1663, 2157};  
static const int skin_threshold = 1570636;                    


static const int y_low = 20;
static const int y_high = 220;


static int evaluate_skin_color_difference(int cb, int cr) {
  const int cb_q6 = cb << 6;
  const int cr_q6 = cr << 6;
  const int cb_diff_q12 = (cb_q6 - skin_mean[0]) * (cb_q6 - skin_mean[0]);
  const int cbcr_diff_q12 = (cb_q6 - skin_mean[0]) * (cr_q6 - skin_mean[1]);
  const int cr_diff_q12 = (cr_q6 - skin_mean[1]) * (cr_q6 - skin_mean[1]);
  const int cb_diff_q2 = (cb_diff_q12 + (1 << 9)) >> 10;
  const int cbcr_diff_q2 = (cbcr_diff_q12 + (1 << 9)) >> 10;
  const int cr_diff_q2 = (cr_diff_q12 + (1 << 9)) >> 10;
  const int skin_diff = skin_inv_cov[0] * cb_diff_q2 +
      skin_inv_cov[1] * cbcr_diff_q2 +
      skin_inv_cov[2] * cbcr_diff_q2 +
      skin_inv_cov[3] * cr_diff_q2;
  return skin_diff;
}

int vp9_skin_pixel(const uint8_t y, const uint8_t cb, const uint8_t cr) {
  if (y < y_low || y > y_high)
    return 0;
  else
    return (evaluate_skin_color_difference(cb, cr) < skin_threshold);
}

#ifdef OUTPUT_YUV_SKINMAP

void vp9_compute_skin_map(VP9_COMP *const cpi, FILE *yuv_skinmap_file) {
  int i, j, mi_row, mi_col;
  VP9_COMMON *const cm = &cpi->common;
  uint8_t *y;
  const uint8_t *src_y = cpi->Source->y_buffer;
  const uint8_t *src_u = cpi->Source->u_buffer;
  const uint8_t *src_v = cpi->Source->v_buffer;
  const int src_ystride = cpi->Source->y_stride;
  const int src_uvstride = cpi->Source->uv_stride;
  YV12_BUFFER_CONFIG skinmap;
  memset(&skinmap, 0, sizeof(YV12_BUFFER_CONFIG));
  if (vp9_alloc_frame_buffer(&skinmap, cm->width, cm->height,
                               cm->subsampling_x, cm->subsampling_y,
                               VP9_ENC_BORDER_IN_PIXELS, cm->byte_alignment)) {
      vp9_free_frame_buffer(&skinmap);
      return;
  }
  memset(skinmap.buffer_alloc, 128, skinmap.frame_size);
  y = skinmap.y_buffer;
  
  
  
  for (mi_row = 0; mi_row < cm->mi_rows - 1; ++mi_row) {
    for (mi_col = 0; mi_col < cm->mi_cols - 1; ++mi_col) {
      
      
      const uint8_t ysource = src_y[4 * src_ystride + 4];
      const uint8_t usource = src_u[2 * src_uvstride + 2];
      const uint8_t vsource = src_v[2 * src_uvstride + 2];
      const int is_skin = vp9_skin_pixel(ysource, usource, vsource);
      for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
          if (is_skin)
            y[i * src_ystride + j] = 255;
          else
            y[i * src_ystride + j] = src_y[i * src_ystride + j];
        }
      }
      y += 8;
      src_y += 8;
      src_u += 4;
      src_v += 4;
    }
    y += (src_ystride << 3) - ((cm->mi_cols - 1) << 3);
    src_y += (src_ystride << 3) - ((cm->mi_cols - 1) << 3);
    src_u += (src_uvstride << 2) - ((cm->mi_cols - 1) << 2);
    src_v += (src_uvstride << 2) - ((cm->mi_cols - 1) << 2);
  }
  vp9_write_yuv_frame_420(&skinmap, yuv_skinmap_file);
  vp9_free_frame_buffer(&skinmap);
}
#endif
