









#include "./vpx_rtcd.h"

extern void vp8_yv12_copy_frame_func_neon(struct yv12_buffer_config *src_ybc,
                                          struct yv12_buffer_config *dst_ybc);

void vp8_yv12_copy_frame_neon(struct yv12_buffer_config *src_ybc,
                              struct yv12_buffer_config *dst_ybc) {
  vp8_yv12_copy_frame_func_neon(src_ybc, dst_ybc);

  vp8_yv12_extend_frame_borders_neon(dst_ybc);
}
