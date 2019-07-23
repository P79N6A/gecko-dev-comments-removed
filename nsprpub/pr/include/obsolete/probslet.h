








































#if defined(PROBSLET_H)
#else
#define PROBSLET_H

#include "prio.h"
#include "private/pprio.h"  

PR_BEGIN_EXTERN_C





NSPR_API(PRStatus) PR_Yield(void);











#ifndef PR_MAX_SELECT_DESC
#define PR_MAX_SELECT_DESC 1024
#endif
typedef struct PR_fd_set {
    PRUint32      hsize;
    PRFileDesc   *harray[PR_MAX_SELECT_DESC];
    PRUint32      nsize;
    PROsfd        narray[PR_MAX_SELECT_DESC];
} PR_fd_set;
























































NSPR_API(PRInt32) PR_Select(
    PRInt32 num, PR_fd_set *readfds, PR_fd_set *writefds,
    PR_fd_set *exceptfds, PRIntervalTime timeout);



















NSPR_API(void)        PR_FD_ZERO(PR_fd_set *set);
NSPR_API(void)        PR_FD_SET(PRFileDesc *fd, PR_fd_set *set);
NSPR_API(void)        PR_FD_CLR(PRFileDesc *fd, PR_fd_set *set);
NSPR_API(PRInt32)     PR_FD_ISSET(PRFileDesc *fd, PR_fd_set *set);
NSPR_API(void)        PR_FD_NSET(PROsfd osfd, PR_fd_set *set);
NSPR_API(void)        PR_FD_NCLR(PROsfd osfd, PR_fd_set *set);
NSPR_API(PRInt32)     PR_FD_NISSET(PROsfd osfd, PR_fd_set *set);






NSPR_API(PRInt32) PR_GetSysfdTableMax(void);

NSPR_API(PRInt32) PR_SetSysfdTableSize(PRIntn table_size);

#ifndef NO_NSPR_10_SUPPORT
#include <sys/stat.h>

NSPR_API(PRInt32) PR_Stat(const char *path, struct stat *buf);
#endif 

PR_END_EXTERN_C

#endif 


