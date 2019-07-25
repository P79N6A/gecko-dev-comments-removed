










#ifndef VPX_PORTS_ARM_H
#define VPX_PORTS_ARM_H
#include <stdlib.h>
#include "config.h"


#define HAS_EDSP  0x01

#define HAS_MEDIA 0x02

#define HAS_NEON  0x04

int arm_cpu_caps(void);

#endif

