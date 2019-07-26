






































#ifndef _CPR_MEMORY_H_
#define _CPR_MEMORY_H_

#include "cpr_types.h"

__BEGIN_DECLS




typedef void *cprBuffer_t;

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_memory.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_memory.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_memory.h"
#endif

















#if defined SIP_OS_WINDOWS
#define cprGetBuffer(y)  cpr_calloc(1,y)
#else
void *cprGetBuffer(uint32_t size);
#endif












void cprReleaseBuffer(void *bufferPtr);












void *cprGetSysHeader(void *bufferPtr);










void cprReleaseSysHeader(void *sysHdrPtr);

__END_DECLS

#endif
