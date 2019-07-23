






































#ifndef nsLeakDetector_h
#define nsLeakDetector_h

#ifndef nsError_h
#include "nsError.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

nsresult NS_InitGarbageCollector(void);
nsresult NS_InitLeakDetector(void);
nsresult NS_ShutdownLeakDetector(void);
nsresult NS_ShutdownGarbageCollector(void);

#ifdef __cplusplus
} 
#endif

#endif
