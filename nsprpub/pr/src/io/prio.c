




































#include "primpl.h"

#include <string.h> 




PRLock *_pr_flock_lock;
PRCondVar *_pr_flock_cv;

#ifdef WINCE




#define STD_INPUT_HANDLE  ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE  ((DWORD)-12)

static HANDLE GetStdHandle(DWORD nStdHandle)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}
#endif

void _PR_InitIO(void)
{
    const PRIOMethods *methods = PR_GetFileMethods();

    _PR_InitFdCache();

    _pr_flock_lock = PR_NewLock();
    _pr_flock_cv = PR_NewCondVar(_pr_flock_lock);

#ifdef WIN32
    _pr_stdin = PR_AllocFileDesc((PROsfd)GetStdHandle(STD_INPUT_HANDLE),
            methods);
    _pr_stdout = PR_AllocFileDesc((PROsfd)GetStdHandle(STD_OUTPUT_HANDLE),
            methods);
    _pr_stderr = PR_AllocFileDesc((PROsfd)GetStdHandle(STD_ERROR_HANDLE),
            methods);
#ifdef WINNT
    _pr_stdin->secret->md.sync_file_io = PR_TRUE;
    _pr_stdout->secret->md.sync_file_io = PR_TRUE;
    _pr_stderr->secret->md.sync_file_io = PR_TRUE;
#endif
#else
    _pr_stdin = PR_AllocFileDesc(0, methods);
    _pr_stdout = PR_AllocFileDesc(1, methods);
    _pr_stderr = PR_AllocFileDesc(2, methods);
#endif
    _PR_MD_INIT_FD_INHERITABLE(_pr_stdin, PR_TRUE);
    _PR_MD_INIT_FD_INHERITABLE(_pr_stdout, PR_TRUE);
    _PR_MD_INIT_FD_INHERITABLE(_pr_stderr, PR_TRUE);

    _PR_MD_INIT_IO();
}

void _PR_CleanupIO(void)
{
    PR_FreeFileDesc(_pr_stdin);
    _pr_stdin = NULL;
    PR_FreeFileDesc(_pr_stdout);
    _pr_stdout = NULL;
    PR_FreeFileDesc(_pr_stderr);
    _pr_stderr = NULL;

    if (_pr_flock_cv) {
        PR_DestroyCondVar(_pr_flock_cv);
        _pr_flock_cv = NULL;
    }
    if (_pr_flock_lock) {
        PR_DestroyLock(_pr_flock_lock);
        _pr_flock_lock = NULL;
    }

    _PR_CleanupFdCache();
}

PR_IMPLEMENT(PRFileDesc*) PR_GetSpecialFD(PRSpecialFD osfd)
{
    PRFileDesc *result = NULL;
    PR_ASSERT((int) osfd >= PR_StandardInput && osfd <= PR_StandardError);

    if (!_pr_initialized) _PR_ImplicitInitialization();
    
    switch (osfd)
    {
        case PR_StandardInput: result = _pr_stdin; break;
        case PR_StandardOutput: result = _pr_stdout; break;
        case PR_StandardError: result = _pr_stderr; break;
        default:
            (void)PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    }
    return result;
}

PR_IMPLEMENT(PRFileDesc*) PR_AllocFileDesc(
    PROsfd osfd, const PRIOMethods *methods)
{
    PRFileDesc *fd;

#ifdef XP_UNIX
	



	PR_ASSERT(osfd < FD_SETSIZE);
#endif
    fd = _PR_Getfd();
    if (fd) {
        
        fd->methods = methods;
        fd->secret->state = _PR_FILEDESC_OPEN;
	fd->secret->md.osfd = osfd;
        _PR_MD_INIT_FILEDESC(fd);
    } else {
	    PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    }

    return fd;
}

PR_IMPLEMENT(void) PR_FreeFileDesc(PRFileDesc *fd)
{
    PR_ASSERT(fd);
    _PR_Putfd(fd);
}




PR_IMPLEMENT(PRInt32) PR_Poll(PRPollDesc *pds, PRIntn npds, PRIntervalTime timeout)
{
	return(_PR_MD_PR_POLL(pds, npds, timeout));
}




PR_IMPLEMENT(PRStatus) PR_SetFDInheritable(
    PRFileDesc *fd,
    PRBool inheritable)
{
#if defined(XP_UNIX) || defined(WIN32) || defined(XP_OS2) || defined(XP_BEOS)
    



    if (fd->identity != PR_NSPR_IO_LAYER) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }
    if (fd->secret->inheritable != inheritable) {
        if (_PR_MD_SET_FD_INHERITABLE(fd, inheritable) == PR_FAILURE) {
            return PR_FAILURE;
        }
        fd->secret->inheritable = inheritable;
    }
    return PR_SUCCESS;
#else
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
#endif
}





PR_IMPLEMENT(void) PT_FPrintStats(PRFileDesc *debug_out, const char *msg)
{
    
}  
