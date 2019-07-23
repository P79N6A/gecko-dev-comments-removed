










































#include "primpl.h"

#include <string.h>














static void _pr_ConvertSemName(char *result)
{
#ifdef _PR_HAVE_POSIX_SEMAPHORES
#if defined(SOLARIS)
    char *p;

    
    for (p = result+1; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
    return;
#else
    return;
#endif
#elif defined(_PR_HAVE_SYSV_SEMAPHORES)
    return;
#elif defined(WIN32)
    return;
#endif
}

static void _pr_ConvertShmName(char *result)
{
#if defined(PR_HAVE_POSIX_NAMED_SHARED_MEMORY)
#if defined(SOLARIS)
    char *p;

    
    for (p = result+1; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
    return;
#else
    return;
#endif
#elif defined(PR_HAVE_SYSV_NAMED_SHARED_MEMORY)
    return;
#elif defined(WIN32)
    return;
#else
    return;
#endif
}

PRStatus _PR_MakeNativeIPCName(
    const char *name,
    char *result,
    PRIntn size,
    _PRIPCType type)
{
    if (strlen(name) >= (PRSize)size) {
        PR_SetError(PR_BUFFER_OVERFLOW_ERROR, 0);
        return PR_FAILURE;
    }
    strcpy(result, name);
    switch (type) {
        case _PRIPCSem:
            _pr_ConvertSemName(result);
            break;
        case _PRIPCShm:
            _pr_ConvertShmName(result);
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            return PR_FAILURE;
    }
    return PR_SUCCESS;
}
