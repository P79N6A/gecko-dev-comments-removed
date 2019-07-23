




































#ifndef prsystem_h___
#define prsystem_h___




#include "prtypes.h"

PR_BEGIN_EXTERN_C







NSPR_API(char) PR_GetDirectorySeparator(void);






NSPR_API(char) PR_GetDirectorySepartor(void);







NSPR_API(char) PR_GetPathSeparator(void);


typedef enum {
    PR_SI_HOSTNAME,  

    PR_SI_SYSNAME,
    PR_SI_RELEASE,
    PR_SI_ARCHITECTURE,
    PR_SI_HOSTNAME_UNTRUNCATED  

} PRSysInfo;











#define SYS_INFO_BUFFER_LENGTH 256

NSPR_API(PRStatus) PR_GetSystemInfo(PRSysInfo cmd, char *buf, PRUint32 buflen);




NSPR_API(PRInt32) PR_GetPageSize(void);




NSPR_API(PRInt32) PR_GetPageShift(void);















NSPR_API(PRInt32) PR_GetNumberOfProcessors( void );















NSPR_API(PRUint64) PR_GetPhysicalMemorySize(void);

PR_END_EXTERN_C

#endif 
