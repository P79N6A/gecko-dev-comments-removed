









#ifndef VP8_COMMON_SYSTEMDEPENDENT_H_
#define VP8_COMMON_SYSTEMDEPENDENT_H_

#include "vpx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct VP8Common;
void vp8_machine_specific_config(struct VP8Common *);

#ifdef __cplusplus
}  
#endif

#endif
