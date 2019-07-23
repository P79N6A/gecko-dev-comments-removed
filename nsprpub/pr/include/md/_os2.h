




































#ifndef nspr_os2_defs_h___
#define nspr_os2_defs_h___

#ifndef NO_LONG_LONG
#define INCL_LONGLONG
#endif
#define INCL_DOS
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_WPS
#include <os2.h>
#include <sys/select.h>

#include "prio.h"

#include <errno.h>





#define PR_LINKER_ARCH      "os2"
#define _PR_SI_SYSNAME        "OS2"
#define _PR_SI_ARCHITECTURE   "x86"    /* XXXMB hardcode for now */

#define HAVE_DLL
#define _PR_GLOBAL_THREADS_ONLY
#undef  HAVE_THREAD_AFFINITY
#define _PR_HAVE_THREADSAFE_GETHOST
#define _PR_HAVE_ATOMIC_OPS
#define HAVE_NETINET_TCP_H

#define HANDLE unsigned long
#define HINSTANCE HMODULE




extern struct PRLock                      *_pr_schedLock;


typedef void (*FiberFunc)(void *);

#define PR_NUM_GCREGS           8
typedef PRInt32	                PR_CONTEXT_TYPE[PR_NUM_GCREGS];
#define GC_VMBASE               0x40000000
#define GC_VMLIMIT              0x00FFFFFF
typedef int (*FARPROC)();

#define _MD_MAGIC_THREAD	0x22222222
#define _MD_MAGIC_THREADSTACK	0x33333333
#define _MD_MAGIC_SEGMENT	0x44444444
#define _MD_MAGIC_DIR		0x55555555
#define _MD_MAGIC_CV        0x66666666

struct _MDSemaphore {
   HEV sem;
};

struct _MDCPU {
    int              unused;
}; 

struct _MDThread {
    HEV              blocked_sema;      


    PRBool           inCVWaitQueue;     


    TID              handle;            
    void            *sp;                
    PRUint32         magic;             
    PR_CONTEXT_TYPE  gcContext;         
    struct PRThread *prev, *next;       


};

struct _MDThreadStack {
    PRUint32           magic;          
};

struct _MDSegment {
    PRUint32           magic;          
};

#undef PROFILE_LOCKS

struct _MDDir {
    HDIR           d_hdl;
    union {
        FILEFINDBUF3  small;
        FILEFINDBUF3L large;
    } d_entry;
    PRBool           firstEntry;     

    PRUint32         magic;          
};

struct _MDCVar {
    PRUint32 magic;
    struct PRThread *waitHead, *waitTail;  



    PRIntn nwait;                          

};

#define _MD_CV_NOTIFIED_LENGTH 6
typedef struct _MDNotified _MDNotified;
struct _MDNotified {
    PRIntn length;                     

    struct {
        struct _MDCVar *cv;            
        PRIntn times;                  
        struct PRThread *notifyHead;   
    } cv[_MD_CV_NOTIFIED_LENGTH];
    _MDNotified *link;                 
};

struct _MDLock {
    HMTX mutex;                        

    





    struct _MDNotified notified;     
#ifdef PROFILE_LOCKS
    PRInt32 hitcount;
    PRInt32 misscount;
#endif
};

struct _MDFileDesc {
    PRInt32 osfd;    






};

struct _MDProcess {
   PID pid;
};


#define _MD_GET_SP(thread)            (thread)->md.gcContext[6]



#define _MD_OPEN                      (_PR_MD_OPEN)
#define _MD_OPEN_FILE                 (_PR_MD_OPEN)
#define _MD_READ                      (_PR_MD_READ)
#define _MD_WRITE                     (_PR_MD_WRITE)
#define _MD_WRITEV                    (_PR_MD_WRITEV)
#define _MD_LSEEK                     (_PR_MD_LSEEK)
#define _MD_LSEEK64                   (_PR_MD_LSEEK64)
extern PRInt32 _MD_CloseFile(PRInt32 osfd);
#define _MD_CLOSE_FILE                _MD_CloseFile
#define _MD_GETFILEINFO               (_PR_MD_GETFILEINFO)
#define _MD_GETFILEINFO64             (_PR_MD_GETFILEINFO64)
#define _MD_GETOPENFILEINFO           (_PR_MD_GETOPENFILEINFO)
#define _MD_GETOPENFILEINFO64         (_PR_MD_GETOPENFILEINFO64)
#define _MD_STAT                      (_PR_MD_STAT)
#define _MD_RENAME                    (_PR_MD_RENAME)
#define _MD_ACCESS                    (_PR_MD_ACCESS)
#define _MD_DELETE                    (_PR_MD_DELETE)
#define _MD_MKDIR                     (_PR_MD_MKDIR)
#define _MD_MAKE_DIR                  (_PR_MD_MKDIR)
#define _MD_RMDIR                     (_PR_MD_RMDIR)
#define _MD_LOCKFILE                  (_PR_MD_LOCKFILE)
#define _MD_TLOCKFILE                 (_PR_MD_TLOCKFILE)
#define _MD_UNLOCKFILE                (_PR_MD_UNLOCKFILE)




#define _MD_EACCES                EACCES
#define _MD_EADDRINUSE            EADDRINUSE
#define _MD_EADDRNOTAVAIL         EADDRNOTAVAIL
#define _MD_EAFNOSUPPORT          EAFNOSUPPORT
#define _MD_EAGAIN                EWOULDBLOCK
#define _MD_EALREADY              EALREADY
#define _MD_EBADF                 EBADF
#define _MD_ECONNREFUSED          ECONNREFUSED
#define _MD_ECONNRESET            ECONNRESET
#define _MD_EFAULT                SOCEFAULT
#define _MD_EINPROGRESS           EINPROGRESS
#define _MD_EINTR                 EINTR
#define _MD_EINVAL                EINVAL
#define _MD_EISCONN               EISCONN
#define _MD_ENETUNREACH           ENETUNREACH
#define _MD_ENOENT                ENOENT
#define _MD_ENOTCONN              ENOTCONN
#define _MD_ENOTSOCK              ENOTSOCK
#define _MD_EOPNOTSUPP            EOPNOTSUPP
#define _MD_EWOULDBLOCK           EWOULDBLOCK
#define _MD_GET_SOCKET_ERROR()    sock_errno()
#ifndef INADDR_LOOPBACK 

#endif  

#define _MD_INIT_FILEDESC(fd)
extern void _MD_MakeNonblock(PRFileDesc *f);
#define _MD_MAKE_NONBLOCK             _MD_MakeNonblock
#define _MD_INIT_FD_INHERITABLE       (_PR_MD_INIT_FD_INHERITABLE)
#define _MD_QUERY_FD_INHERITABLE      (_PR_MD_QUERY_FD_INHERITABLE)
#define _MD_SHUTDOWN                  (_PR_MD_SHUTDOWN)
#define _MD_LISTEN                    _PR_MD_LISTEN
extern PRInt32 _MD_CloseSocket(PRInt32 osfd);
#define _MD_CLOSE_SOCKET              _MD_CloseSocket
#define _MD_SENDTO                    (_PR_MD_SENDTO)
#define _MD_RECVFROM                  (_PR_MD_RECVFROM)
#define _MD_SOCKETPAIR                (_PR_MD_SOCKETPAIR)
#define _MD_GETSOCKNAME               (_PR_MD_GETSOCKNAME)
#define _MD_GETPEERNAME               (_PR_MD_GETPEERNAME)
#define _MD_GETSOCKOPT                (_PR_MD_GETSOCKOPT)
#define _MD_SETSOCKOPT                (_PR_MD_SETSOCKOPT)

#define _MD_FSYNC                     _PR_MD_FSYNC
#define _MD_SET_FD_INHERITABLE        (_PR_MD_SET_FD_INHERITABLE)

#ifdef _PR_HAVE_ATOMIC_OPS
#define _MD_INIT_ATOMIC()
#define _MD_ATOMIC_INCREMENT          _PR_MD_ATOMIC_INCREMENT
#define _MD_ATOMIC_ADD                _PR_MD_ATOMIC_ADD
#define _MD_ATOMIC_DECREMENT          _PR_MD_ATOMIC_DECREMENT
#define _MD_ATOMIC_SET                _PR_MD_ATOMIC_SET
#endif

#define _MD_INIT_IO                   (_PR_MD_INIT_IO)
#define _MD_PR_POLL                   (_PR_MD_PR_POLL)

#define _MD_SOCKET                    (_PR_MD_SOCKET)
extern PRInt32 _MD_SocketAvailable(PRFileDesc *fd);
#define _MD_SOCKETAVAILABLE           _MD_SocketAvailable
#define _MD_PIPEAVAILABLE             _MD_SocketAvailable
#define _MD_CONNECT                   (_PR_MD_CONNECT)
extern PRInt32 _MD_Accept(PRFileDesc *fd, PRNetAddr *raddr, PRUint32 *rlen,
        PRIntervalTime timeout);
#define _MD_ACCEPT                    _MD_Accept
#define _MD_BIND                      (_PR_MD_BIND)
#define _MD_RECV                      (_PR_MD_RECV)
#define _MD_SEND                      (_PR_MD_SEND)



#define _MD_PAUSE_CPU


#define PR_DIRECTORY_SEPARATOR        '\\'
#define PR_DIRECTORY_SEPARATOR_STR    "\\"
#define PR_PATH_SEPARATOR		';'
#define PR_PATH_SEPARATOR_STR		";"
#define _MD_ERRNO()                   errno
#define _MD_OPEN_DIR                  (_PR_MD_OPEN_DIR)
#define _MD_CLOSE_DIR                 (_PR_MD_CLOSE_DIR)
#define _MD_READ_DIR                  (_PR_MD_READ_DIR)


#define _MD_INIT_SEGS()
#define _MD_ALLOC_SEGMENT(seg, size, vaddr)   0
#define _MD_FREE_SEGMENT(seg)


#define _MD_GET_ENV                 (_PR_MD_GET_ENV)
#define _MD_PUT_ENV                 (_PR_MD_PUT_ENV)


#define _MD_DEFAULT_STACK_SIZE      65536L
#define _MD_INIT_THREAD             (_PR_MD_INIT_THREAD)
#define _MD_INIT_ATTACHED_THREAD    (_PR_MD_INIT_THREAD)
#define _MD_CREATE_THREAD           (_PR_MD_CREATE_THREAD)
#define _MD_YIELD                   (_PR_MD_YIELD)
#define _MD_SET_PRIORITY            (_PR_MD_SET_PRIORITY)
#define _MD_CLEAN_THREAD            (_PR_MD_CLEAN_THREAD)
#define _MD_SETTHREADAFFINITYMASK   (_PR_MD_SETTHREADAFFINITYMASK)
#define _MD_GETTHREADAFFINITYMASK   (_PR_MD_GETTHREADAFFINITYMASK)
#define _MD_EXIT_THREAD             (_PR_MD_EXIT_THREAD)
#define _MD_SUSPEND_THREAD          (_PR_MD_SUSPEND_THREAD)
#define _MD_RESUME_THREAD           (_PR_MD_RESUME_THREAD)
#define _MD_SUSPEND_CPU             (_PR_MD_SUSPEND_CPU)
#define _MD_RESUME_CPU              (_PR_MD_RESUME_CPU)
#define _MD_WAKEUP_CPUS             (_PR_MD_WAKEUP_CPUS)
#define _MD_BEGIN_SUSPEND_ALL()
#define _MD_BEGIN_RESUME_ALL()
#define _MD_END_SUSPEND_ALL()
#define _MD_END_RESUME_ALL()


#define _PR_LOCK                      _MD_LOCK
#define _PR_UNLOCK					  _MD_UNLOCK

#define _MD_NEW_LOCK                  (_PR_MD_NEW_LOCK)
#define _MD_FREE_LOCK(lock)           (DosCloseMutexSem((lock)->mutex))
#define _MD_LOCK(lock)                (DosRequestMutexSem((lock)->mutex, SEM_INDEFINITE_WAIT))
#define _MD_TEST_AND_LOCK(lock)       (DosRequestMutexSem((lock)->mutex, SEM_INDEFINITE_WAIT),0)
#define _MD_UNLOCK                    (_PR_MD_UNLOCK)


#define _MD_WAIT                      (_PR_MD_WAIT)
#define _MD_WAKEUP_WAITER             (_PR_MD_WAKEUP_WAITER)


#define _MD_WAIT_CV					  (_PR_MD_WAIT_CV)
#define _MD_NEW_CV					  (_PR_MD_NEW_CV)
#define _MD_FREE_CV					  (_PR_MD_FREE_CV)
#define _MD_NOTIFY_CV				  (_PR_MD_NOTIFY_CV	)
#define _MD_NOTIFYALL_CV			  (_PR_MD_NOTIFYALL_CV)

   

#define _MD_IOQ_LOCK()                
#define _MD_IOQ_UNLOCK()              



#define _MD_START_INTERRUPTS()
#define _MD_STOP_INTERRUPTS()
#define _MD_DISABLE_CLOCK_INTERRUPTS()
#define _MD_ENABLE_CLOCK_INTERRUPTS()
#define _MD_BLOCK_CLOCK_INTERRUPTS()
#define _MD_UNBLOCK_CLOCK_INTERRUPTS()
#define _MD_EARLY_INIT                (_PR_MD_EARLY_INIT)
#define _MD_FINAL_INIT()
#define _MD_INIT_CPUS()
#define _MD_INIT_RUNNING_CPU(cpu)

struct PRProcess;
struct PRProcessAttr;

#define _MD_CREATE_PROCESS _PR_CreateOS2Process
extern struct PRProcess * _PR_CreateOS2Process(
    const char *path,
    char *const *argv,
    char *const *envp,
    const struct PRProcessAttr *attr
);

#define _MD_DETACH_PROCESS _PR_DetachOS2Process
extern PRStatus _PR_DetachOS2Process(struct PRProcess *process);


#define _MD_WAIT_PROCESS _PR_WaitOS2Process
extern PRStatus _PR_WaitOS2Process(struct PRProcess *process, 
    PRInt32 *exitCode);

#define _MD_KILL_PROCESS _PR_KillOS2Process
extern PRStatus _PR_KillOS2Process(struct PRProcess *process);

#define _MD_CLEANUP_BEFORE_EXIT()
#define _MD_EXIT                          (_PR_MD_EXIT)
#define _MD_INIT_CONTEXT(_thread, _sp, _main, status) \
    PR_BEGIN_MACRO \
    *status = PR_TRUE; \
    PR_END_MACRO
#define _MD_SWITCH_CONTEXT
#define _MD_RESTORE_CONTEXT


#define _MD_INTERVAL_INIT                 (_PR_MD_INTERVAL_INIT)
#define _MD_GET_INTERVAL                  (_PR_MD_GET_INTERVAL)
#define _MD_INTERVAL_PER_SEC              (_PR_MD_INTERVAL_PER_SEC)
#define _MD_INTERVAL_PER_MILLISEC()       (_PR_MD_INTERVAL_PER_SEC() / 1000)
#define _MD_INTERVAL_PER_MICROSEC()       (_PR_MD_INTERVAL_PER_SEC() / 1000000)



typedef struct __NSPR_TLS
{
    struct PRThread  *_pr_thread_last_run;
    struct PRThread  *_pr_currentThread;
    struct _PRCPU    *_pr_currentCPU;
} _NSPR_TLS;

extern _NSPR_TLS*  pThreadLocalStorage;
NSPR_API(void) _PR_MD_ENSURE_TLS(void);

#define _MD_GET_ATTACHED_THREAD() pThreadLocalStorage->_pr_currentThread
extern struct PRThread * _MD_CURRENT_THREAD(void);
#define _MD_SET_CURRENT_THREAD(_thread) _PR_MD_ENSURE_TLS(); pThreadLocalStorage->_pr_currentThread = (_thread)

#define _MD_LAST_THREAD() pThreadLocalStorage->_pr_thread_last_run
#define _MD_SET_LAST_THREAD(_thread) _PR_MD_ENSURE_TLS(); pThreadLocalStorage->_pr_thread_last_run = (_thread)

#define _MD_CURRENT_CPU() pThreadLocalStorage->_pr_currentCPU
#define _MD_SET_CURRENT_CPU(_cpu) _PR_MD_ENSURE_TLS(); pThreadLocalStorage->_pr_currentCPU = (_cpu)







#define LOCK_SCHEDULER()                 0
#define UNLOCK_SCHEDULER()               0
#define _PR_LockSched()                	 0
#define _PR_UnlockSched()                0


#define _MD_INIT_LOCKS()


#define _MD_INIT_STACK(stack, redzone)
#define _MD_CLEAR_STACK(stack)





struct _MDFileMap {
    PROffset64  maxExtent;
};

extern PRStatus _MD_CreateFileMap(struct PRFileMap *fmap, PRInt64 size);
#define _MD_CREATE_FILE_MAP _MD_CreateFileMap

extern PRInt32 _MD_GetMemMapAlignment(void);
#define _MD_GET_MEM_MAP_ALIGNMENT _MD_GetMemMapAlignment

extern void * _MD_MemMap(struct PRFileMap *fmap, PRInt64 offset,
        PRUint32 len);
#define _MD_MEM_MAP _MD_MemMap

extern PRStatus _MD_MemUnmap(void *addr, PRUint32 size);
#define _MD_MEM_UNMAP _MD_MemUnmap

extern PRStatus _MD_CloseFileMap(struct PRFileMap *fmap);
#define _MD_CLOSE_FILE_MAP _MD_CloseFileMap


typedef ULONG DWORD, *PDWORD;



#ifndef CONTEXT_CONTROL
#define CONTEXT_CONTROL        0x00000001
#define CONTEXT_INTEGER        0x00000002
#define CONTEXT_SEGMENTS       0x00000004
#define CONTEXT_FLOATING_POINT 0x00000008
#define CONTEXT_FULL           0x0000000F

#pragma pack(2)
typedef struct _FPREG {
    ULONG      losig;    
    ULONG      hisig;    
    USHORT     signexp;  
} FPREG;
typedef struct _CONTEXTRECORD {
    ULONG     ContextFlags;
    ULONG     ctx_env[7];
    FPREG     ctx_stack[8];
    ULONG     ctx_SegGs;     
    ULONG     ctx_SegFs;     
    ULONG     ctx_SegEs;     
    ULONG     ctx_SegDs;     
    ULONG     ctx_RegEdi;    
    ULONG     ctx_RegEsi;    
    ULONG     ctx_RegEax;    
    ULONG     ctx_RegEbx;    
    ULONG     ctx_RegEcx;    
    ULONG     ctx_RegEdx;    
    ULONG     ctx_RegEbp;    
    ULONG     ctx_RegEip;    
    ULONG     ctx_SegCs;     
    ULONG     ctx_EFlags;    
    ULONG     ctx_RegEsp;    
    ULONG     ctx_SegSs;     
} CONTEXTRECORD, *PCONTEXTRECORD;
#pragma pack()
#endif

extern APIRET (* APIENTRY QueryThreadContext)(TID, ULONG, PCONTEXTRECORD);










#define FreeLibrary(x) DosFreeModule((HMODULE)x)
#define OutputDebugStringA(x)
                               
extern int _MD_os2_get_nonblocking_connect_error(int osfd);

#endif 
