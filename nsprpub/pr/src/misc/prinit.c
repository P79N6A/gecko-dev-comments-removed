




































#include "primpl.h"
#include <ctype.h>
#include <string.h>

PRLogModuleInfo *_pr_clock_lm;
PRLogModuleInfo *_pr_cmon_lm;
PRLogModuleInfo *_pr_io_lm;
PRLogModuleInfo *_pr_cvar_lm;
PRLogModuleInfo *_pr_mon_lm;
PRLogModuleInfo *_pr_linker_lm;
PRLogModuleInfo *_pr_sched_lm;
PRLogModuleInfo *_pr_thread_lm;
PRLogModuleInfo *_pr_gc_lm;
PRLogModuleInfo *_pr_shm_lm;
PRLogModuleInfo *_pr_shma_lm;

PRFileDesc *_pr_stdin;
PRFileDesc *_pr_stdout;
PRFileDesc *_pr_stderr;

#if !defined(_PR_PTHREADS) && !defined(_PR_BTHREADS)

PRCList _pr_active_local_threadQ =
			PR_INIT_STATIC_CLIST(&_pr_active_local_threadQ);
PRCList _pr_active_global_threadQ =
			PR_INIT_STATIC_CLIST(&_pr_active_global_threadQ);

_MDLock  _pr_cpuLock;           
PRCList _pr_cpuQ = PR_INIT_STATIC_CLIST(&_pr_cpuQ);

PRUint32 _pr_utid;

PRInt32 _pr_userActive;
PRInt32 _pr_systemActive;
PRUintn _pr_maxPTDs;

#ifdef _PR_LOCAL_THREADS_ONLY

struct _PRCPU 	*_pr_currentCPU;
PRThread 	*_pr_currentThread;
PRThread 	*_pr_lastThread;
PRInt32 	_pr_intsOff;

#endif 


PRLock *_pr_terminationCVLock;

#endif 

PRLock *_pr_sleeplock;  

static void _PR_InitCallOnce(void);

PRBool _pr_initialized = PR_FALSE;


PR_IMPLEMENT(PRBool) PR_VersionCheck(const char *importedVersion)
{
    








    int vmajor = 0, vminor = 0, vpatch = 0;
    const char *ptr = importedVersion;

    while (isdigit(*ptr)) {
        vmajor = 10 * vmajor + *ptr - '0';
        ptr++;
    }
    if (*ptr == '.') {
        ptr++;
        while (isdigit(*ptr)) {
            vminor = 10 * vminor + *ptr - '0';
            ptr++;
        }
        if (*ptr == '.') {
            ptr++;
            while (isdigit(*ptr)) {
                vpatch = 10 * vpatch + *ptr - '0';
                ptr++;
            }
        }
    }

    if (vmajor != PR_VMAJOR) {
        return PR_FALSE;
    }
    if (vmajor == PR_VMAJOR && vminor > PR_VMINOR) {
        return PR_FALSE;
    }
    if (vmajor == PR_VMAJOR && vminor == PR_VMINOR && vpatch > PR_VPATCH) {
        return PR_FALSE;
    }
    return PR_TRUE;
}  


PR_IMPLEMENT(PRBool) PR_Initialized(void)
{
    return _pr_initialized;
}

PRInt32 _native_threads_only = 0;

#ifdef WINNT
static void _pr_SetNativeThreadsOnlyMode(void)
{
    HMODULE mainExe;
    PRBool *globalp;
    char *envp;

    mainExe = GetModuleHandle(NULL);
    PR_ASSERT(NULL != mainExe);
    globalp = (PRBool *) GetProcAddress(mainExe, "nspr_native_threads_only");
    if (globalp) {
        _native_threads_only = (*globalp != PR_FALSE);
    } else if (envp = getenv("NSPR_NATIVE_THREADS_ONLY")) {
        _native_threads_only = (atoi(envp) == 1);
    }
}
#endif

static void _PR_InitStuff(void)
{

    if (_pr_initialized) return;
    _pr_initialized = PR_TRUE;
#ifdef _PR_ZONE_ALLOCATOR
    _PR_InitZones();
#endif
#ifdef WINNT
    _pr_SetNativeThreadsOnlyMode();
#endif


    (void) PR_GetPageSize();

	_pr_clock_lm = PR_NewLogModule("clock");
	_pr_cmon_lm = PR_NewLogModule("cmon");
	_pr_io_lm = PR_NewLogModule("io");
	_pr_mon_lm = PR_NewLogModule("mon");
	_pr_linker_lm = PR_NewLogModule("linker");
	_pr_cvar_lm = PR_NewLogModule("cvar");
	_pr_sched_lm = PR_NewLogModule("sched");
	_pr_thread_lm = PR_NewLogModule("thread");
	_pr_gc_lm = PR_NewLogModule("gc");
	_pr_shm_lm = PR_NewLogModule("shm");
	_pr_shma_lm = PR_NewLogModule("shma");
      
     
    _PR_MD_EARLY_INIT();

    _PR_InitLocks();
    _PR_InitAtomic();
    _PR_InitSegs();
    _PR_InitStacks();
	_PR_InitTPD();
    _PR_InitEnv();
    _PR_InitLayerCache();
    _PR_InitClock();

    _pr_sleeplock = PR_NewLock();
    PR_ASSERT(NULL != _pr_sleeplock);

#ifdef GC_LEAK_DETECTOR
    _PR_InitGarbageCollector();
#endif

    _PR_InitThreads(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    
#ifdef WIN16
	{
	PRInt32 top;   
    _pr_top_of_task_stack = (char *) &top;
	}
#endif    

#ifndef _PR_GLOBAL_THREADS_ONLY
	_PR_InitCPUs();
#endif





#ifdef _PR_OVERRIDE_MALLOC
    _PR_InitMem();
#endif

    _PR_InitCMon();
    _PR_InitIO();
    _PR_InitNet();
    _PR_InitTime();
    _PR_InitLog();
    _PR_InitLinker();
    _PR_InitCallOnce();
    _PR_InitDtoa();
    _PR_InitMW();
    _PR_InitRWLocks();

    nspr_InitializePRErrorTable();

    _PR_MD_FINAL_INIT();
}

void _PR_ImplicitInitialization(void)
{
	_PR_InitStuff();

    
#if !defined(_PR_PTHREADS) && !defined(_PR_GLOBAL_THREADS_ONLY)
    _PR_MD_START_INTERRUPTS();
#endif

}

PR_IMPLEMENT(void) PR_DisableClockInterrupts(void)
{
#if !defined(_PR_PTHREADS) && !defined(_PR_BTHREADS)
	if (!_pr_initialized) {
		_PR_InitStuff();
	} else {
    	_PR_MD_DISABLE_CLOCK_INTERRUPTS();
	}
#endif
}

PR_IMPLEMENT(void) PR_EnableClockInterrupts(void)
{
#if !defined(_PR_PTHREADS) && !defined(_PR_BTHREADS)
	if (!_pr_initialized) {
		_PR_InitStuff();
	}
    _PR_MD_ENABLE_CLOCK_INTERRUPTS();
#endif
}

PR_IMPLEMENT(void) PR_BlockClockInterrupts(void)
{
#if !defined(_PR_PTHREADS) && !defined(_PR_BTHREADS)
    	_PR_MD_BLOCK_CLOCK_INTERRUPTS();
#endif
}

PR_IMPLEMENT(void) PR_UnblockClockInterrupts(void)
{
#if !defined(_PR_PTHREADS) && !defined(_PR_BTHREADS)
    	_PR_MD_UNBLOCK_CLOCK_INTERRUPTS();
#endif
}

PR_IMPLEMENT(void) PR_Init(
    PRThreadType type, PRThreadPriority priority, PRUintn maxPTDs)
{
    _PR_ImplicitInitialization();
}

PR_IMPLEMENT(PRIntn) PR_Initialize(
    PRPrimordialFn prmain, PRIntn argc, char **argv, PRUintn maxPTDs)
{
    PRIntn rv;
    _PR_ImplicitInitialization();
    rv = prmain(argc, argv);
	PR_Cleanup();
    return rv;
}  















#if defined(_PR_PTHREADS) || defined(_PR_BTHREADS)
    
#else
static void
_PR_CleanupBeforeExit(void)
{





    _PR_CleanupTPD();
    if (_pr_terminationCVLock)
    



        PR_DestroyLock(_pr_terminationCVLock);

    _PR_MD_CLEANUP_BEFORE_EXIT();
}
#endif 

























#if defined(_PR_PTHREADS) || defined(_PR_BTHREADS)
    
#else

PR_IMPLEMENT(PRStatus) PR_Cleanup()
{
    PRThread *me = PR_GetCurrentThread();
    PR_ASSERT((NULL != me) && (me->flags & _PR_PRIMORDIAL));
    if ((NULL != me) && (me->flags & _PR_PRIMORDIAL))
    {
        PR_LOG(_pr_thread_lm, PR_LOG_MIN, ("PR_Cleanup: shutting down NSPR"));

        


        _pr_recycleThreads = 0;

        



        PR_Lock(_pr_activeLock);
        while (_pr_userActive > _pr_primordialExitCount) {
            PR_WaitCondVar(_pr_primordialExitCVar, PR_INTERVAL_NO_TIMEOUT);
        }
        if (me->flags & _PR_SYSTEM) {
            _pr_systemActive--;
        } else {
            _pr_userActive--;
        }
        PR_Unlock(_pr_activeLock);

#ifdef IRIX
		_PR_MD_PRE_CLEANUP(me);
		


    	PR_ASSERT((_PR_IS_NATIVE_THREAD(me)) || (me->cpu->id == 0));
#endif

        _PR_CleanupMW();
        _PR_CleanupTime();
        _PR_CleanupDtoa();
        _PR_CleanupCallOnce();
		_PR_ShutdownLinker();
        _PR_CleanupNet();
        _PR_CleanupIO();
        
        _PR_CleanupThread(me);

        _PR_MD_STOP_INTERRUPTS();

	    PR_LOG(_pr_thread_lm, PR_LOG_MIN,
	            ("PR_Cleanup: clean up before destroying thread"));
	    _PR_LogCleanup();

        



        if (_PR_IS_NATIVE_THREAD(me)) {
            _PR_MD_EXIT_THREAD(me);
            _PR_NativeDestroyThread(me);
        } else {
            _PR_UserDestroyThread(me);
            PR_DELETE(me->stack);
            PR_DELETE(me);
        }

        






#ifdef WINNT
        _PR_CleanupCPUs();
#endif
        _PR_CleanupThreads();
        _PR_CleanupCMon();
        PR_DestroyLock(_pr_sleeplock);
        _pr_sleeplock = NULL;
        _PR_CleanupLayerCache();
        _PR_CleanupEnv();
        _PR_CleanupStacks();
        _PR_CleanupBeforeExit();
        _pr_initialized = PR_FALSE;
        return PR_SUCCESS;
    }
    return PR_FAILURE;
}
#endif 














#if defined(_PR_PTHREADS) || defined(_PR_BTHREADS)
    
#else
PR_IMPLEMENT(void) PR_ProcessExit(PRIntn status)
{
    _PR_MD_EXIT(status);
}

#endif 

PR_IMPLEMENT(PRProcessAttr *)
PR_NewProcessAttr(void)
{
    PRProcessAttr *attr;

    attr = PR_NEWZAP(PRProcessAttr);
    if (!attr) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    }
    return attr;
}

PR_IMPLEMENT(void)
PR_ResetProcessAttr(PRProcessAttr *attr)
{
    PR_FREEIF(attr->currentDirectory);
    PR_FREEIF(attr->fdInheritBuffer);
    memset(attr, 0, sizeof(*attr));
}

PR_IMPLEMENT(void)
PR_DestroyProcessAttr(PRProcessAttr *attr)
{
    PR_FREEIF(attr->currentDirectory);
    PR_FREEIF(attr->fdInheritBuffer);
    PR_DELETE(attr);
}

PR_IMPLEMENT(void)
PR_ProcessAttrSetStdioRedirect(
    PRProcessAttr *attr,
    PRSpecialFD stdioFd,
    PRFileDesc *redirectFd)
{
    switch (stdioFd) {
        case PR_StandardInput:
            attr->stdinFd = redirectFd;
            break;
        case PR_StandardOutput:
            attr->stdoutFd = redirectFd;
            break;
        case PR_StandardError:
            attr->stderrFd = redirectFd;
            break;
        default:
            PR_ASSERT(0);
    }
}




PR_IMPLEMENT(void)
PR_SetStdioRedirect(
    PRProcessAttr *attr,
    PRSpecialFD stdioFd,
    PRFileDesc *redirectFd)
{
#if defined(DEBUG)
    static PRBool warn = PR_TRUE;
    if (warn) {
        warn = _PR_Obsolete("PR_SetStdioRedirect()",
                "PR_ProcessAttrSetStdioRedirect()");
    }
#endif
    PR_ProcessAttrSetStdioRedirect(attr, stdioFd, redirectFd);
}

PR_IMPLEMENT(PRStatus)
PR_ProcessAttrSetCurrentDirectory(
    PRProcessAttr *attr,
    const char *dir)
{
    PR_FREEIF(attr->currentDirectory);
    attr->currentDirectory = (char *) PR_MALLOC(strlen(dir) + 1);
    if (!attr->currentDirectory) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return PR_FAILURE;
    }
    strcpy(attr->currentDirectory, dir);
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus)
PR_ProcessAttrSetInheritableFD(
    PRProcessAttr *attr,
    PRFileDesc *fd,
    const char *name)
{
    
#define FD_INHERIT_BUFFER_INCR 128
    
#define NSPR_INHERIT_FDS_STRLEN 17
    
#ifdef _WIN64
#define OSFD_STRLEN 18
#else
#define OSFD_STRLEN 10
#endif
    
#define FD_TYPE_STRLEN 1
    PRSize newSize;
    int remainder;
    char *newBuffer;
    int nwritten;
    char *cur;
    int freeSize;

    if (fd->identity != PR_NSPR_IO_LAYER) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }
    if (fd->secret->inheritable == _PR_TRI_UNKNOWN) {
        _PR_MD_QUERY_FD_INHERITABLE(fd);
    }
    if (fd->secret->inheritable != _PR_TRI_TRUE) {
        PR_SetError(PR_NO_ACCESS_RIGHTS_ERROR, 0);
        return PR_FAILURE;
    }

    



    if (NULL == attr->fdInheritBuffer) {
        
        newSize = NSPR_INHERIT_FDS_STRLEN + strlen(name)
                + FD_TYPE_STRLEN + OSFD_STRLEN + 2 + 1;
    } else {
        
        newSize = attr->fdInheritBufferUsed + strlen(name)
                + FD_TYPE_STRLEN + OSFD_STRLEN + 3 + 1;
    }
    if (newSize > attr->fdInheritBufferSize) {
        
        remainder = newSize % FD_INHERIT_BUFFER_INCR;
        if (remainder != 0) {
            newSize += (FD_INHERIT_BUFFER_INCR - remainder);
        }
        if (NULL == attr->fdInheritBuffer) {
            newBuffer = (char *) PR_MALLOC(newSize);
        } else {
            newBuffer = (char *) PR_REALLOC(attr->fdInheritBuffer, newSize);
        }
        if (NULL == newBuffer) {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            return PR_FAILURE;
        }
        attr->fdInheritBuffer = newBuffer;
        attr->fdInheritBufferSize = newSize;
    }
    cur = attr->fdInheritBuffer + attr->fdInheritBufferUsed;
    freeSize = attr->fdInheritBufferSize - attr->fdInheritBufferUsed;
    if (0 == attr->fdInheritBufferUsed) {
        nwritten = PR_snprintf(cur, freeSize,
                "NSPR_INHERIT_FDS=%s:%d:0x%" PR_PRIxOSFD,
                name, (PRIntn)fd->methods->file_type, fd->secret->md.osfd);
    } else {
        nwritten = PR_snprintf(cur, freeSize, ":%s:%d:0x%" PR_PRIxOSFD,
                name, (PRIntn)fd->methods->file_type, fd->secret->md.osfd);
    }
    attr->fdInheritBufferUsed += nwritten; 
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRFileDesc *) PR_GetInheritedFD(
    const char *name)
{
    PRFileDesc *fd;
    const char *envVar;
    const char *ptr;
    int len = strlen(name);
    PROsfd osfd;
    int nColons;
    PRIntn fileType;

    envVar = PR_GetEnv("NSPR_INHERIT_FDS");
    if (NULL == envVar || '\0' == envVar[0]) {
        PR_SetError(PR_UNKNOWN_ERROR, 0);
        return NULL;
    }

    ptr = envVar;
    while (1) {
        if ((ptr[len] == ':') && (strncmp(ptr, name, len) == 0)) {
            ptr += len + 1;
            PR_sscanf(ptr, "%d:0x%" PR_SCNxOSFD, &fileType, &osfd);
            switch ((PRDescType)fileType) {
                case PR_DESC_FILE:
                    fd = PR_ImportFile(osfd);
                    break;
                case PR_DESC_PIPE:
                    fd = PR_ImportPipe(osfd);
                    break;
                case PR_DESC_SOCKET_TCP:
                    fd = PR_ImportTCPSocket(osfd);
                    break;
                case PR_DESC_SOCKET_UDP:
                    fd = PR_ImportUDPSocket(osfd);
                    break;
                default:
                    PR_ASSERT(0);
                    PR_SetError(PR_UNKNOWN_ERROR, 0);
                    fd = NULL;
                    break;
            }
            if (fd) {
                




                fd->secret->inheritable = _PR_TRI_TRUE;
            }
            return fd;
        }
        
        nColons = 0;
        while (*ptr) {
            if (*ptr == ':') {
                if (++nColons == 3) {
                    break;
                }
            }
            ptr++;
        }
        if (*ptr == '\0') {
            PR_SetError(PR_UNKNOWN_ERROR, 0);
            return NULL;
        }
        ptr++;
    }
}

PR_IMPLEMENT(PRProcess*) PR_CreateProcess(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr)
{
    return _PR_MD_CREATE_PROCESS(path, argv, envp, attr);
}  

PR_IMPLEMENT(PRStatus) PR_CreateProcessDetached(
    const char *path,
    char *const *argv,
    char *const *envp,
    const PRProcessAttr *attr)
{
    PRProcess *process;
    PRStatus rv;

    process = PR_CreateProcess(path, argv, envp, attr);
    if (NULL == process) {
	return PR_FAILURE;
    }
    rv = PR_DetachProcess(process);
    PR_ASSERT(PR_SUCCESS == rv);
    if (rv == PR_FAILURE) {
	PR_DELETE(process);
	return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) PR_DetachProcess(PRProcess *process)
{
    return _PR_MD_DETACH_PROCESS(process);
}

PR_IMPLEMENT(PRStatus) PR_WaitProcess(PRProcess *process, PRInt32 *exitCode)
{
    return _PR_MD_WAIT_PROCESS(process, exitCode);
}  

PR_IMPLEMENT(PRStatus) PR_KillProcess(PRProcess *process)
{
    return _PR_MD_KILL_PROCESS(process);
}









static struct {
    PRLock *ml;
    PRCondVar *cv;
} mod_init;

static void _PR_InitCallOnce(void) {
    mod_init.ml = PR_NewLock();
    PR_ASSERT(NULL != mod_init.ml);
    mod_init.cv = PR_NewCondVar(mod_init.ml);
    PR_ASSERT(NULL != mod_init.cv);
}

void _PR_CleanupCallOnce()
{
    PR_DestroyLock(mod_init.ml);
    mod_init.ml = NULL;
    PR_DestroyCondVar(mod_init.cv);
    mod_init.cv = NULL;
}

PR_IMPLEMENT(PRStatus) PR_CallOnce(
    PRCallOnceType *once,
    PRCallOnceFN    func)
{
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (!once->initialized) {
	if (PR_AtomicSet(&once->inProgress, 1) == 0) {
	    once->status = (*func)();
	    PR_Lock(mod_init.ml);
	    once->initialized = 1;
	    PR_NotifyAllCondVar(mod_init.cv);
	    PR_Unlock(mod_init.ml);
	} else {
	    PR_Lock(mod_init.ml);
	    while (!once->initialized) {
		PR_WaitCondVar(mod_init.cv, PR_INTERVAL_NO_TIMEOUT);
            }
	    PR_Unlock(mod_init.ml);
	}
    } else {
        if (PR_SUCCESS != once->status) {
            PR_SetError(PR_CALL_ONCE_ERROR, 0);
        }
    }
    return once->status;
}

PR_IMPLEMENT(PRStatus) PR_CallOnceWithArg(
    PRCallOnceType      *once,
    PRCallOnceWithArgFN  func,
    void                *arg)
{
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (!once->initialized) {
	if (PR_AtomicSet(&once->inProgress, 1) == 0) {
	    once->status = (*func)(arg);
	    PR_Lock(mod_init.ml);
	    once->initialized = 1;
	    PR_NotifyAllCondVar(mod_init.cv);
	    PR_Unlock(mod_init.ml);
	} else {
	    PR_Lock(mod_init.ml);
	    while (!once->initialized) {
		PR_WaitCondVar(mod_init.cv, PR_INTERVAL_NO_TIMEOUT);
            }
	    PR_Unlock(mod_init.ml);
	}
    } else {
        if (PR_SUCCESS != once->status) {
            PR_SetError(PR_CALL_ONCE_ERROR, 0);
        }
    }
    return once->status;
}

PRBool _PR_Obsolete(const char *obsolete, const char *preferred)
{
#if defined(DEBUG)
    PR_fprintf(
        PR_STDERR, "'%s' is obsolete. Use '%s' instead.\n",
        obsolete, (NULL == preferred) ? "something else" : preferred);
#endif
    return PR_FALSE;
}  




