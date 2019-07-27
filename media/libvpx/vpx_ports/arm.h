










#ifndef VPX_PORTS_ARM_H_
#define VPX_PORTS_ARM_H_
#include <stdlib.h>
#include "vpx_config.h"

#ifdef __cplusplus
extern "C" {
#endif


#define HAS_EDSP  0x01

#define HAS_MEDIA 0x02

#define HAS_NEON  0x04

int arm_cpu_caps(void);


#if !defined(__clang__) && defined(__GNUC__) && \
    __GNUC__ == 4 && __GNUC_MINOR__ <= 6
#define VPX_INCOMPATIBLE_GCC
#endif

#ifdef __cplusplus
}  
#endif

#endif

