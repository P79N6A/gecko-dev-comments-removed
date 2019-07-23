




































#include "primpl.h"
#include <sys/timeb.h>





PRProcess * _PR_CreateWindowsProcess(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

PRStatus _PR_DetachWindowsProcess(PRProcess *process)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PRStatus _PR_WaitWindowsProcess(PRProcess *process,
    PRInt32 *exitCode)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PRStatus _PR_KillWindowsProcess(PRProcess *process)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

