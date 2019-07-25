










#include "vpx_config.h"
#include "vpx/vpx_codec.h"
#include "vpx_ports/asm_offsets.h"
#include "vpx_scale/yv12config.h"

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




#if HAVE_ARMV7

ct_assert(VP8BORDERINPIXELS_VAL, VP8BORDERINPIXELS == 32)
#endif
