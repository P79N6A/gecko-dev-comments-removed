









#include "./vp9_rtcd.h"
#include "./vpx_config.h"
#include "vp9/common/x86/convolve.h"

#if HAVE_SSE2
filter8_1dfunction vp9_filter_block1d16_v8_sse2;
filter8_1dfunction vp9_filter_block1d16_h8_sse2;
filter8_1dfunction vp9_filter_block1d8_v8_sse2;
filter8_1dfunction vp9_filter_block1d8_h8_sse2;
filter8_1dfunction vp9_filter_block1d4_v8_sse2;
filter8_1dfunction vp9_filter_block1d4_h8_sse2;
filter8_1dfunction vp9_filter_block1d16_v8_avg_sse2;
filter8_1dfunction vp9_filter_block1d16_h8_avg_sse2;
filter8_1dfunction vp9_filter_block1d8_v8_avg_sse2;
filter8_1dfunction vp9_filter_block1d8_h8_avg_sse2;
filter8_1dfunction vp9_filter_block1d4_v8_avg_sse2;
filter8_1dfunction vp9_filter_block1d4_h8_avg_sse2;

filter8_1dfunction vp9_filter_block1d16_v2_sse2;
filter8_1dfunction vp9_filter_block1d16_h2_sse2;
filter8_1dfunction vp9_filter_block1d8_v2_sse2;
filter8_1dfunction vp9_filter_block1d8_h2_sse2;
filter8_1dfunction vp9_filter_block1d4_v2_sse2;
filter8_1dfunction vp9_filter_block1d4_h2_sse2;
filter8_1dfunction vp9_filter_block1d16_v2_avg_sse2;
filter8_1dfunction vp9_filter_block1d16_h2_avg_sse2;
filter8_1dfunction vp9_filter_block1d8_v2_avg_sse2;
filter8_1dfunction vp9_filter_block1d8_h2_avg_sse2;
filter8_1dfunction vp9_filter_block1d4_v2_avg_sse2;
filter8_1dfunction vp9_filter_block1d4_h2_avg_sse2;





















FUN_CONV_1D(horiz, x_step_q4, filter_x, h, src, , sse2);
FUN_CONV_1D(vert, y_step_q4, filter_y, v, src - src_stride * 3, , sse2);
FUN_CONV_1D(avg_horiz, x_step_q4, filter_x, h, src, avg_, sse2);
FUN_CONV_1D(avg_vert, y_step_q4, filter_y, v, src - src_stride * 3, avg_, sse2);











FUN_CONV_2D(, sse2);
FUN_CONV_2D(avg_ , sse2);

#if CONFIG_VP9_HIGHBITDEPTH && ARCH_X86_64
highbd_filter8_1dfunction vp9_highbd_filter_block1d16_v8_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d16_h8_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_v8_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_h8_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_v8_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_h8_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d16_v8_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d16_h8_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_v8_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_h8_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_v8_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_h8_avg_sse2;

highbd_filter8_1dfunction vp9_highbd_filter_block1d16_v2_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d16_h2_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_v2_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_h2_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_v2_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_h2_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d16_v2_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d16_h2_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_v2_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d8_h2_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_v2_avg_sse2;
highbd_filter8_1dfunction vp9_highbd_filter_block1d4_h2_avg_sse2;





































HIGH_FUN_CONV_1D(horiz, x_step_q4, filter_x, h, src, , sse2);
HIGH_FUN_CONV_1D(vert, y_step_q4, filter_y, v, src - src_stride * 3, , sse2);
HIGH_FUN_CONV_1D(avg_horiz, x_step_q4, filter_x, h, src, avg_, sse2);
HIGH_FUN_CONV_1D(avg_vert, y_step_q4, filter_y, v, src - src_stride * 3, avg_,
                 sse2);











HIGH_FUN_CONV_2D(, sse2);
HIGH_FUN_CONV_2D(avg_ , sse2);
#endif  
#endif  
