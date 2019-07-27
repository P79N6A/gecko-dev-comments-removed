










#ifndef VP9_COMMON_VP9_ALLOCCOMMON_H_
#define VP9_COMMON_VP9_ALLOCCOMMON_H_

#include "vp9/common/vp9_onyxc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp9_initialize_common();

void vp9_update_mode_info_border(VP9_COMMON *cm, MODE_INFO *mi);

void vp9_create_common(VP9_COMMON *cm);
void vp9_remove_common(VP9_COMMON *cm);

int vp9_resize_frame_buffers(VP9_COMMON *cm, int width, int height);
int vp9_alloc_frame_buffers(VP9_COMMON *cm, int width, int height);
void vp9_free_frame_buffers(VP9_COMMON *cm);


void vp9_update_frame_size(VP9_COMMON *cm);

#ifdef __cplusplus
}  
#endif

#endif
