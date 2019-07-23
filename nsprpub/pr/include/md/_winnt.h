




































#ifndef nspr_win32_defs_h___
#define nspr_win32_defs_h___



 
#ifndef  _WIN32_WINNT
    #define _WIN32_WINNT 0x0400
#elif   (_WIN32_WINNT < 0x0400)
    #undef  _WIN32_WINNT
    #define _WIN32_WINNT 0x0400
#endif 

#include <windows.h>
#include <winsock.h>
#ifdef __MINGW32__
#include <mswsock.h>
#endif
#include <errno.h>

#include "prio.h"
#include "prclist.h"





#define PR_LINKER_ARCH      "win32"
#define _PR_SI_SYSNAME        "WINNT"
#if defined(_M_IX86) || defined(_X86_)
#define _PR_SI_ARCHITECTURE   "x86"
#elif defined(_M_X64) || defined(_M_AMD64) || defined(_AMD64_)
#define _PR_SI_ARCHITECTURE   "x86-64"
#elif defined(_M_IA64) || defined(_IA64_)
#define _PR_SI_ARCHITECTURE   "ia64"
#else
#error unknown processor architecture
#endif

#define HAVE_DLL
#define HAVE_CUSTOM_USER_THREADS
#define HAVE_THREAD_AFFINITY
#define _PR_HAVE_GETADDRINFO
#define _PR_INET6_PROBE
#ifndef _PR_INET6
#define AF_INET6 23

#ifndef AI_CANONNAME
#define AI_CANONNAME 0x2
#define AI_NUMERICHOST 0x4
#define NI_NUMERICHOST 0x02
struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    size_t ai_addrlen;
    char *ai_canonname;
    struct sockaddr *ai_addr;
    struct addrinfo *ai_next;
};
#endif
#define _PR_HAVE_MD_SOCKADDR_IN6

struct _md_in6_addr {
    union {
        PRUint8  _S6_u8[16];
        PRUint16 _S6_u16[8];
    } _S6_un;
};

struct _md_sockaddr_in6 {
    PRInt16 sin6_family;
    PRUint16 sin6_port;
    PRUint32 sin6_flowinfo;
    struct _md_in6_addr sin6_addr;
    PRUint32 sin6_scope_id;
};
#endif
#define _PR_HAVE_THREADSAFE_GETHOST
#define _PR_HAVE_ATOMIC_OPS
#if defined(_M_IX86) || defined(_X86_)
#define _PR_HAVE_ATOMIC_CAS
#endif
#define PR_HAVE_WIN32_NAMED_SHARED_MEMORY
#define _PR_HAVE_PEEK_BUFFER
#define _PR_PEEK_BUFFER_MAX (32 * 1024)
#define _PR_FD_NEED_EMULATE_MSG_PEEK(fd) \
    (!(fd)->secret->nonblocking && (fd)->secret->inheritable != _PR_TRI_TRUE)
#define _PR_NEED_SECRET_AF




extern struct PRLock                      *_pr_schedLock;


typedef void (*FiberFunc)(void *);

#define PR_NUM_GCREGS           8
typedef PRInt32	                PR_CONTEXT_TYPE[PR_NUM_GCREGS];
#define GC_VMBASE               0x40000000
#define GC_VMLIMIT              0x00FFFFFF

#define _MD_MAGIC_THREAD	0x22222222
#define _MD_MAGIC_THREADSTACK	0x33333333
#define _MD_MAGIC_SEGMENT	0x44444444
#define _MD_MAGIC_DIR		0x55555555

struct _MDCPU {
    int              unused;
};

enum _MDIOModel {
    _MD_BlockingIO = 0x38,
    _MD_MultiWaitIO = 0x49
};

typedef struct _MDOverlapped {
    OVERLAPPED overlapped;              

    enum _MDIOModel ioModel;            


    union {
        struct _MDThread *mdThread;     



        struct {
            PRCList links;              
            struct PRRecvWait *desc;    



            struct PRWaitGroup *group;
            struct TimerEvent *timer;
            DWORD error;
        } mw;
    } data;
} _MDOverlapped;

struct _MDThread {
        
    struct _MDOverlapped overlapped;    
    void            *acceptex_buf;      
    TRANSMIT_FILE_BUFFERS *xmit_bufs;   
    HANDLE           blocked_sema;      


    PRInt32          blocked_io_status; 
    PRInt32          blocked_io_bytes;  
    PRInt32          blocked_io_error;  
    HANDLE           handle;
    PRUint32         id;
    void            *sp;                
    PRUint32         magic;             
    PR_CONTEXT_TYPE  gcContext;         
	struct _PRCPU    *thr_bound_cpu;		
	PRBool   		 interrupt_disabled;
	HANDLE 			 thr_event;			


    
    void            *fiber_id;          
    FiberFunc        fiber_fn;          
    void            *fiber_arg;         
    PRUint32         fiber_stacksize;   
    PRInt32          fiber_last_error;  
    void (*start)(void *);              


};

struct _MDThreadStack {
    PRUint32           magic;          
};

struct _MDSegment {
    PRUint32           magic;          
};

#undef PROFILE_LOCKS

struct _MDLock {
    CRITICAL_SECTION mutex;          
#ifdef PROFILE_LOCKS
    PRInt32 hitcount;
    PRInt32 misscount;
#endif
};

struct _MDDir {
    HANDLE           d_hdl;
    WIN32_FIND_DATA  d_entry;
    PRBool           firstEntry;     

    PRUint32         magic;          
};

struct _MDCVar {
    PRUint32         unused;
};

struct _MDSemaphore {
    HANDLE           sem;
};

struct _MDFileDesc {
    PROsfd osfd;     







    PRBool io_model_committed;  




    PRBool sync_file_io;        

    PRBool accepted_socket;     

    PRNetAddr peer_addr;        





};

struct _MDProcess {
    HANDLE handle;
    DWORD id;
};



#define _MD_GET_SP(thread)            (thread)->md.gcContext[6]



extern void _PR_NT_InitSids(void);
extern void _PR_NT_FreeSids(void);
extern PRStatus _PR_NT_MakeSecurityDescriptorACL(
    PRIntn mode,
    DWORD accessTable[],
    PSECURITY_DESCRIPTOR *resultSD,
    PACL *resultACL
);
extern void _PR_NT_FreeSecurityDescriptorACL(
    PSECURITY_DESCRIPTOR pSD, PACL pACL);



extern PRInt32 _md_Associate(HANDLE);
extern PRInt32 _PR_MD_CLOSE(PROsfd osfd, PRBool socket);

#define _MD_OPEN                      _PR_MD_OPEN
#define _MD_OPEN_FILE                 _PR_MD_OPEN_FILE
#define _MD_READ                      _PR_MD_READ
#define _MD_WRITE                     _PR_MD_WRITE
#define _MD_WRITEV                    _PR_MD_WRITEV
#define _MD_LSEEK                     _PR_MD_LSEEK
#define _MD_LSEEK64                   _PR_MD_LSEEK64
#define _MD_CLOSE_FILE(f)             _PR_MD_CLOSE(f, PR_FALSE)
#define _MD_GETFILEINFO               _PR_MD_GETFILEINFO
#define _MD_GETFILEINFO64             _PR_MD_GETFILEINFO64
#define _MD_GETOPENFILEINFO           _PR_MD_GETOPENFILEINFO
#define _MD_GETOPENFILEINFO64         _PR_MD_GETOPENFILEINFO64
#define _MD_STAT                      _PR_MD_STAT
#define _MD_RENAME                    _PR_MD_RENAME     
#define _MD_ACCESS                    _PR_MD_ACCESS     
#define _MD_DELETE                    _PR_MD_DELETE     
#define _MD_MKDIR                     _PR_MD_MKDIR      
#define _MD_MAKE_DIR                  _PR_MD_MAKE_DIR
#define _MD_RMDIR                     _PR_MD_RMDIR      
#define _MD_LOCKFILE                  _PR_MD_LOCKFILE
#define _MD_TLOCKFILE                 _PR_MD_TLOCKFILE
#define _MD_UNLOCKFILE                _PR_MD_UNLOCKFILE


#define _MD_GET_SOCKET_ERROR()    WSAGetLastError()
#define _MD_SET_SOCKET_ERROR(_err) WSASetLastError(_err)

#define _MD_INIT_FILEDESC(fd)
#define _MD_MAKE_NONBLOCK             _PR_MD_MAKE_NONBLOCK
#define _MD_INIT_FD_INHERITABLE       _PR_MD_INIT_FD_INHERITABLE
#define _MD_QUERY_FD_INHERITABLE      _PR_MD_QUERY_FD_INHERITABLE
#define _MD_SHUTDOWN                  _PR_MD_SHUTDOWN
#define _MD_LISTEN                    _PR_MD_LISTEN
#define _MD_CLOSE_SOCKET(s)           _PR_MD_CLOSE(s, PR_TRUE)
#define _MD_SENDTO                    _PR_MD_SENDTO
#define _MD_RECVFROM                  _PR_MD_RECVFROM
#define _MD_SOCKETPAIR(s, type, proto, sv) -1
#define _MD_GETSOCKNAME               _PR_MD_GETSOCKNAME
#define _MD_GETPEERNAME               _PR_MD_GETPEERNAME
#define _MD_GETSOCKOPT                _PR_MD_GETSOCKOPT
#define _MD_SETSOCKOPT                _PR_MD_SETSOCKOPT
#define _MD_SELECT                    select
extern int _PR_NTFiberSafeSelect(int, fd_set *, fd_set *, fd_set *,
    const struct timeval *);
#define _MD_FSYNC                     _PR_MD_FSYNC
#define _MD_SOCKETAVAILABLE           _PR_MD_SOCKETAVAILABLE
#define _MD_PIPEAVAILABLE             _PR_MD_PIPEAVAILABLE
#define _MD_SET_FD_INHERITABLE        _PR_MD_SET_FD_INHERITABLE

#define _MD_INIT_ATOMIC()
#if defined(_M_IX86) || defined(_X86_)
#define _MD_ATOMIC_INCREMENT          _PR_MD_ATOMIC_INCREMENT
#define _MD_ATOMIC_ADD          	  _PR_MD_ATOMIC_ADD
#define _MD_ATOMIC_DECREMENT          _PR_MD_ATOMIC_DECREMENT
#else 
#define _MD_ATOMIC_INCREMENT(x)       InterlockedIncrement((PLONG)x)
#define _MD_ATOMIC_ADD(ptr,val)    (InterlockedExchangeAdd((PLONG)ptr, (LONG)val) + val)
#define _MD_ATOMIC_DECREMENT(x)       InterlockedDecrement((PLONG)x)
#endif 
#define _MD_ATOMIC_SET(x,y)           InterlockedExchange((PLONG)x, (LONG)y)

#define _MD_INIT_IO                   _PR_MD_INIT_IO
#define _MD_SOCKET                    _PR_MD_SOCKET
#define _MD_CONNECT                   _PR_MD_CONNECT

#define _MD_ACCEPT(s, a, l, to)       \
        _MD_FAST_ACCEPT(s, a, l, to, PR_FALSE, NULL, NULL)
#define _MD_FAST_ACCEPT(s, a, l, to, fast, cb, cba) \
        _PR_MD_FAST_ACCEPT(s, a, l, to, fast, cb, cba)
#define _MD_ACCEPT_READ(s, ns, ra, buf, l, t) \
        _MD_FAST_ACCEPT_READ(s, ns, ra, buf, l, t, PR_FALSE, NULL, NULL)
#define _MD_FAST_ACCEPT_READ(s, ns, ra, buf, l, t, fast, cb, cba) \
        _PR_MD_FAST_ACCEPT_READ(s, ns, ra, buf, l, t, fast, cb, cba)
#define _MD_UPDATE_ACCEPT_CONTEXT     _PR_MD_UPDATE_ACCEPT_CONTEXT

#define _MD_BIND                      _PR_MD_BIND
#define _MD_RECV                      _PR_MD_RECV
#define _MD_SEND                      _PR_MD_SEND
#define _MD_SENDFILE              	  _PR_MD_SENDFILE
#define _MD_PR_POLL                   _PR_MD_PR_POLL


#define _MD_PAUSE_CPU                   _PR_MD_PAUSE_CPU


#define PR_DIRECTORY_SEPARATOR        '\\'
#define PR_DIRECTORY_SEPARATOR_STR    "\\"
#define PR_PATH_SEPARATOR		';'
#define PR_PATH_SEPARATOR_STR		";"
#define _MD_ERRNO()                   GetLastError()
#define _MD_OPEN_DIR                  _PR_MD_OPEN_DIR
#define _MD_CLOSE_DIR                 _PR_MD_CLOSE_DIR
#define _MD_READ_DIR                  _PR_MD_READ_DIR


#define _MD_INIT_SEGS()
#define _MD_ALLOC_SEGMENT(seg, size, vaddr)   0
#define _MD_FREE_SEGMENT(seg)


#define _MD_GET_ENV                 _PR_MD_GET_ENV
#define _MD_PUT_ENV                 _PR_MD_PUT_ENV


#define _MD_DEFAULT_STACK_SIZE            0
#define _MD_INIT_THREAD             _PR_MD_INIT_THREAD
#define _MD_INIT_ATTACHED_THREAD    _PR_MD_INIT_THREAD
#define _MD_CREATE_THREAD           _PR_MD_CREATE_THREAD
#define _MD_JOIN_THREAD             _PR_MD_JOIN_THREAD
#define _MD_END_THREAD              _PR_MD_END_THREAD
#define _MD_YIELD                   _PR_MD_YIELD
#define _MD_SET_PRIORITY            _PR_MD_SET_PRIORITY
#define _MD_CLEAN_THREAD            _PR_MD_CLEAN_THREAD
#define _MD_SETTHREADAFFINITYMASK   _PR_MD_SETTHREADAFFINITYMASK
#define _MD_GETTHREADAFFINITYMASK   _PR_MD_GETTHREADAFFINITYMASK
#define _MD_EXIT_THREAD             _PR_MD_EXIT_THREAD
#define _MD_SUSPEND_THREAD          _PR_MD_SUSPEND_THREAD
#define _MD_RESUME_THREAD           _PR_MD_RESUME_THREAD
#define _MD_SUSPEND_CPU             _PR_MD_SUSPEND_CPU
#define _MD_RESUME_CPU              _PR_MD_RESUME_CPU
#define _MD_BEGIN_SUSPEND_ALL()
#define _MD_BEGIN_RESUME_ALL()
#define _MD_END_SUSPEND_ALL()
#define _MD_END_RESUME_ALL()

extern void _PR_Unblock_IO_Wait(PRThread *thr);


#define _MD_NEW_LOCK(lock)            (InitializeCriticalSection(&((lock)->mutex)),PR_SUCCESS)
#define _MD_FREE_LOCK(lock)           DeleteCriticalSection(&((lock)->mutex))
#ifndef PROFILE_LOCKS
#define _MD_LOCK(lock)                EnterCriticalSection(&((lock)->mutex))
#define _MD_TEST_AND_LOCK(lock)       (TryEnterCriticalSection(&((lock)->mutex))== FALSE)
#define _MD_UNLOCK(lock)              LeaveCriticalSection(&((lock)->mutex))
#else
#define _MD_LOCK(lock)                 \
    PR_BEGIN_MACRO \
    BOOL rv = TryEnterCriticalSection(&((lock)->mutex)); \
    if (rv == TRUE) { \
        InterlockedIncrement(&((lock)->hitcount)); \
    } else { \
        InterlockedIncrement(&((lock)->misscount)); \
        EnterCriticalSection(&((lock)->mutex)); \
    } \
    PR_END_MACRO
#define _MD_TEST_AND_LOCK(lock)       0  /* XXXMB */
#define _MD_UNLOCK(lock)              LeaveCriticalSection(&((lock)->mutex))
#endif
#define _PR_LOCK                      _MD_LOCK
#define _PR_UNLOCK					  _MD_UNLOCK


#define _MD_WAIT                      _PR_MD_WAIT
#define _MD_WAKEUP_WAITER             _PR_MD_WAKEUP_WAITER

   
extern  struct _MDLock              _pr_ioq_lock;
#define _MD_IOQ_LOCK()                _MD_LOCK(&_pr_ioq_lock)
#define _MD_IOQ_UNLOCK()              _MD_UNLOCK(&_pr_ioq_lock)



#define _MD_START_INTERRUPTS()
#define _MD_STOP_INTERRUPTS()
#define _MD_DISABLE_CLOCK_INTERRUPTS()
#define _MD_ENABLE_CLOCK_INTERRUPTS()
#define _MD_BLOCK_CLOCK_INTERRUPTS()
#define _MD_UNBLOCK_CLOCK_INTERRUPTS()
#define _MD_EARLY_INIT                _PR_MD_EARLY_INIT
#define _MD_FINAL_INIT()
#define _MD_INIT_CPUS()
#define _MD_INIT_RUNNING_CPU(cpu)

struct PRProcess;
struct PRProcessAttr;


#define _MD_CREATE_PROCESS _PR_CreateWindowsProcess
extern struct PRProcess * _PR_CreateWindowsProcess(
    const char *path,
    char *const *argv,
    char *const *envp,
    const struct PRProcessAttr *attr
);

#define _MD_DETACH_PROCESS _PR_DetachWindowsProcess
extern PRStatus _PR_DetachWindowsProcess(struct PRProcess *process);


#define _MD_WAIT_PROCESS _PR_WaitWindowsProcess
extern PRStatus _PR_WaitWindowsProcess(struct PRProcess *process, 
    PRInt32 *exitCode);

#define _MD_KILL_PROCESS _PR_KillWindowsProcess
extern PRStatus _PR_KillWindowsProcess(struct PRProcess *process);


#define HAVE_FIBERS
#define _MD_CREATE_USER_THREAD            _PR_MD_CREATE_USER_THREAD
#define _MD_CREATE_PRIMORDIAL_USER_THREAD _PR_MD_CREATE_PRIMORDIAL_USER_THREAD
#define _MD_CLEANUP_BEFORE_EXIT           _PR_MD_CLEANUP_BEFORE_EXIT
#define _MD_EXIT                          _PR_MD_EXIT
#define _MD_INIT_CONTEXT                  _PR_MD_INIT_CONTEXT
#define _MD_SWITCH_CONTEXT                _PR_MD_SWITCH_CONTEXT
#define _MD_RESTORE_CONTEXT               _PR_MD_RESTORE_CONTEXT


#define _MD_INTERVAL_INIT                 _PR_MD_INTERVAL_INIT
#define _MD_GET_INTERVAL                  _PR_MD_GET_INTERVAL
#define _MD_INTERVAL_PER_SEC              _PR_MD_INTERVAL_PER_SEC
#define _MD_INTERVAL_PER_MILLISEC()       (_PR_MD_INTERVAL_PER_SEC() / 1000)
#define _MD_INTERVAL_PER_MICROSEC()       (_PR_MD_INTERVAL_PER_SEC() / 1000000)


extern void _PR_FileTimeToPRTime(const FILETIME *filetime, PRTime *prtm);



extern BOOL _pr_use_static_tls;

extern __declspec(thread) struct PRThread *_pr_current_fiber;
extern DWORD _pr_currentFiberIndex;

#define _MD_GET_ATTACHED_THREAD() \
    (_pr_use_static_tls ? _pr_current_fiber \
    : (PRThread *) TlsGetValue(_pr_currentFiberIndex))

extern struct PRThread * _MD_CURRENT_THREAD(void);

#define _MD_SET_CURRENT_THREAD(_thread) \
    PR_BEGIN_MACRO \
        if (_pr_use_static_tls) { \
            _pr_current_fiber = (_thread); \
        } else { \
            TlsSetValue(_pr_currentFiberIndex, (_thread)); \
        } \
    PR_END_MACRO

extern __declspec(thread) struct PRThread *_pr_fiber_last_run;
extern DWORD _pr_lastFiberIndex;

#define _MD_LAST_THREAD() \
    (_pr_use_static_tls ? _pr_fiber_last_run \
    : (PRThread *) TlsGetValue(_pr_lastFiberIndex))

#define _MD_SET_LAST_THREAD(_thread) \
    PR_BEGIN_MACRO \
        if (_pr_use_static_tls) { \
            _pr_fiber_last_run = (_thread); \
        } else { \
            TlsSetValue(_pr_lastFiberIndex, (_thread)); \
        } \
    PR_END_MACRO

extern __declspec(thread) struct _PRCPU *_pr_current_cpu;
extern DWORD _pr_currentCPUIndex;

#define _MD_CURRENT_CPU() \
    (_pr_use_static_tls ? _pr_current_cpu \
    : (struct _PRCPU *) TlsGetValue(_pr_currentCPUIndex))

#define _MD_SET_CURRENT_CPU(_cpu) \
    PR_BEGIN_MACRO \
        if (_pr_use_static_tls) { \
            _pr_current_cpu = (_cpu); \
        } else { \
            TlsSetValue(_pr_currentCPUIndex, (_cpu)); \
        } \
    PR_END_MACRO

extern __declspec(thread) PRUintn _pr_ints_off;
extern DWORD _pr_intsOffIndex;

#define _MD_GET_INTSOFF() \
    (_pr_use_static_tls ? _pr_ints_off \
    : (PRUintn) TlsGetValue(_pr_intsOffIndex))

#define _MD_SET_INTSOFF(_val) \
    PR_BEGIN_MACRO \
        if (_pr_use_static_tls) { \
            _pr_ints_off = (_val); \
        } else { \
            TlsSetValue(_pr_intsOffIndex, (LPVOID) (_val)); \
        } \
    PR_END_MACRO


#define _MD_INIT_LOCKS()


#define _MD_INIT_STACK(stack, redzone)
#define _MD_CLEAR_STACK(stack)



struct _MDFileMap {
    HANDLE hFileMap;
    DWORD dwAccess;
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


#define _PR_HAVE_NAMED_SEMAPHORES
#define _MD_OPEN_SEMAPHORE            _PR_MD_OPEN_SEMAPHORE
#define _MD_WAIT_SEMAPHORE            _PR_MD_WAIT_SEMAPHORE
#define _MD_POST_SEMAPHORE            _PR_MD_POST_SEMAPHORE
#define _MD_CLOSE_SEMAPHORE           _PR_MD_CLOSE_SEMAPHORE
#define _MD_DELETE_SEMAPHORE(name)    PR_SUCCESS  /* no op */

#endif 
