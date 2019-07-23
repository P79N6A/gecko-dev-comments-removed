


































#ifndef jprof_h___
#define jprof_h___
#include "nscore.h"

#ifdef _IMPL_JPPROF_API
#define JPROF_API(type) NS_EXPORT_(type)
#else
#define JPROF_API(type) NS_IMPORT_(type)
#endif

JPROF_API(void) setupProfilingStuff(void);

#endif 
