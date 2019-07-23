










































#include "plerror.h"

#include "prprf.h"
#include "prerror.h"

PR_IMPLEMENT(void) PL_FPrintError(PRFileDesc *fd, const char *msg)
{
PRErrorCode error = PR_GetError();
PRInt32 oserror = PR_GetOSError();
const char *name = PR_ErrorToName(error);

	if (NULL != msg) PR_fprintf(fd, "%s: ", msg);
    if (NULL == name)
        PR_fprintf(
			fd, " (%d)OUT OF RANGE, oserror = %d\n", error, oserror);
    else
        PR_fprintf(
            fd, "%s(%d), oserror = %d\n",
            name, error, oserror);
}  

PR_IMPLEMENT(void) PL_PrintError(const char *msg)
{
	static PRFileDesc *fd = NULL;
	if (NULL == fd) fd = PR_GetSpecialFD(PR_StandardError);
	PL_FPrintError(fd, msg);
}  


