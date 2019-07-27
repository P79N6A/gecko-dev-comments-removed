










#ifndef VP8_COMMON_ALLOCCOMMON_H_
#define VP8_COMMON_ALLOCCOMMON_H_

#include "onyxc_int.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp8_create_common(VP8_COMMON *oci);
void vp8_remove_common(VP8_COMMON *oci);
void vp8_de_alloc_frame_buffers(VP8_COMMON *oci);
int vp8_alloc_frame_buffers(VP8_COMMON *oci, int width, int height);
void vp8_setup_version(VP8_COMMON *oci);

#ifdef __cplusplus
}  
#endif

#endif
