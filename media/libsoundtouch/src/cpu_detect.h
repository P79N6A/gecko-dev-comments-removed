









































#ifndef _CPU_DETECT_H_
#define _CPU_DETECT_H_

#include "STTypes.h"

#define SUPPORT_MMX         0x0001
#define SUPPORT_3DNOW       0x0002
#define SUPPORT_ALTIVEC     0x0004
#define SUPPORT_SSE         0x0008
#define SUPPORT_SSE2        0x0010




uint detectCPUextensions(void);


void disableExtensions(uint wDisableMask);

#endif  
