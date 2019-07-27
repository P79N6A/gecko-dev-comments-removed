






#ifndef __nssutil_h_
#define __nssutil_h_

#ifndef RC_INVOKED
#include "seccomon.h"
#endif








#define NSSUTIL_VERSION  "3.17.1 Beta"
#define NSSUTIL_VMAJOR   3
#define NSSUTIL_VMINOR   17
#define NSSUTIL_VPATCH   1
#define NSSUTIL_VBUILD   0
#define NSSUTIL_BETA     PR_TRUE

SEC_BEGIN_PROTOS




extern const char *NSSUTIL_GetVersion(void);

extern SECStatus
NSS_InitializePRErrorTable(void);

SEC_END_PROTOS

#endif 
