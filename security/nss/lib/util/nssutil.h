






































#ifndef __nssutil_h_
#define __nssutil_h_

#ifndef RC_INVOKED
#include "prerror.h"
#include "seccomon.h"
#endif








#define NSSUTIL_VERSION  "3.13.0.0 Beta"
#define NSSUTIL_VMAJOR   3
#define NSSUTIL_VMINOR   13
#define NSSUTIL_VPATCH   0
#define NSSUTIL_VBUILD   0
#define NSSUTIL_BETA     PR_TRUE

typedef enum {
    formatSimple = 0,
    formatIncludeErrorCode
} ReportFormatType;
    

SEC_BEGIN_PROTOS




extern const char *NSSUTIL_GetVersion(void);

extern PRStatus
NSS_InitializePRErrorTable(void);



















extern char *
NSS_Strerror(PRErrorCode errNum, ReportFormatType format);






extern char *
NSS_StrerrorTS(PRErrorCode errNum, ReportFormatType format);











extern void
NSS_Perror(const char *s, ReportFormatType format);

SEC_END_PROTOS

#endif 
