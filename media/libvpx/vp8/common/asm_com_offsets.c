










#include "vpx_config.h"
#include "vpx/vpx_codec.h"
#include "vpx_ports/asm_offsets.h"
#include "vpx_scale/yv12config.h"
#include "vp8/common/blockd.h"

BEGIN


DEFINE(yv12_buffer_config_y_width,              offsetof(YV12_BUFFER_CONFIG, y_width));
DEFINE(yv12_buffer_config_y_height,             offsetof(YV12_BUFFER_CONFIG, y_height));
DEFINE(yv12_buffer_config_y_stride,             offsetof(YV12_BUFFER_CONFIG, y_stride));
DEFINE(yv12_buffer_config_uv_width,             offsetof(YV12_BUFFER_CONFIG, uv_width));
DEFINE(yv12_buffer_config_uv_height,            offsetof(YV12_BUFFER_CONFIG, uv_height));
DEFINE(yv12_buffer_config_uv_stride,            offsetof(YV12_BUFFER_CONFIG, uv_stride));
DEFINE(yv12_buffer_config_y_buffer,             offsetof(YV12_BUFFER_CONFIG, y_buffer));
DEFINE(yv12_buffer_config_u_buffer,             offsetof(YV12_BUFFER_CONFIG, u_buffer));
DEFINE(yv12_buffer_config_v_buffer,             offsetof(YV12_BUFFER_CONFIG, v_buffer));
DEFINE(yv12_buffer_config_border,               offsetof(YV12_BUFFER_CONFIG, border));
DEFINE(VP8BORDERINPIXELS_VAL,                   VP8BORDERINPIXELS);

END




#if HAVE_ARMV6

ct_assert(B_DC_PRED, B_DC_PRED == 0);
ct_assert(B_TM_PRED, B_TM_PRED == 1);
ct_assert(B_VE_PRED, B_VE_PRED == 2);
ct_assert(B_HE_PRED, B_HE_PRED == 3);
ct_assert(B_LD_PRED, B_LD_PRED == 4);
ct_assert(B_RD_PRED, B_RD_PRED == 5);
ct_assert(B_VR_PRED, B_VR_PRED == 6);
ct_assert(B_VL_PRED, B_VL_PRED == 7);
ct_assert(B_HD_PRED, B_HD_PRED == 8);
ct_assert(B_HU_PRED, B_HU_PRED == 9);
#endif

#if HAVE_ARMV7

ct_assert(VP8BORDERINPIXELS_VAL, VP8BORDERINPIXELS == 32)
#endif
