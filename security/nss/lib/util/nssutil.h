






































#ifndef __nssutil_h_
#define __nssutil_h_

#ifndef RC_INVOKED
#include "seccomon.h"
#endif








#define NSSUTIL_VERSION  "3.13.2.1"
#define NSSUTIL_VMAJOR   3
#define NSSUTIL_VMINOR   13
#define NSSUTIL_VPATCH   2
#define NSSUTIL_VBUILD   1
#define NSSUTIL_BETA     PR_FALSE

SEC_BEGIN_PROTOS




extern const char *NSSUTIL_GetVersion(void);

extern SECStatus
NSS_InitializePRErrorTable(void);

SEC_END_PROTOS

#endif 
