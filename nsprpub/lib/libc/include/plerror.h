










































#if defined(PLERROR_H)
#else
#define PLERROR_H

#include "prio.h"
#include "prtypes.h"

PR_BEGIN_EXTERN_C



PR_EXTERN(void) PL_PrintError(const char *msg);




PR_EXTERN(void) PL_FPrintError(PRFileDesc *output, const char *msg);

PR_END_EXTERN_C

#endif 


