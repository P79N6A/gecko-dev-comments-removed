





































#ifndef trace_malloc_nsTypeInfo_h_
#define trace_malloc_nsTypeInfo_h_

#include "prtypes.h"

PR_BEGIN_EXTERN_C

extern const char* nsGetTypeName(void* ptr);

extern void RegisterTraceMallocShutdown();

PR_END_EXTERN_C

#endif 
