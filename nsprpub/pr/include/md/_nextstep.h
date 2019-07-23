




































#ifndef nspr_nextstep_defs_h___
#define nspr_nextstep_defs_h___

#include "prthread.h"

#include <bsd/libc.h>
#include <bsd/syscall.h>



extern int syscall(int number, ...);





#define PR_LINKER_ARCH	"nextstep"
#define _PR_SI_SYSNAME  "NEXTSTEP"
#if defined(__sparc__)
#define _PR_SI_ARCHITECTURE "sparc"
#elif defined(__m68k__)
#define _PR_SI_ARCHITECTURE "m68k"
#elif defined(__i386__)
#define _PR_SI_ARCHITECTURE "x86"
#else
error Unknown NEXTSTEP architecture
#endif
#define PR_DLL_SUFFIX		".so"

#define _PR_VMBASE              0x30000000
#define _PR_STACK_VMBASE	0x50000000
#define _MD_DEFAULT_STACK_SIZE	65536L
#define _MD_MMAP_FLAGS          MAP_PRIVATE

#undef  HAVE_STACK_GROWING_UP

#define HAVE_WEAK_MALLOC_SYMBOLS

#define HAVE_DLL
#define USE_MACH_DYLD
#define _PR_STAT_HAS_ONLY_ST_ATIME
#define _PR_NO_LARGE_FILES

#define USE_SETJMP

#ifndef _PR_PTHREADS

#include <setjmp.h>

#define PR_CONTEXT_TYPE	jmp_buf

#define CONTEXT(_th) ((_th)->md.context)






#if defined(__sparc__)
#define _MD_GET_SP(_th)		(_th)->md.context[2]
#elif defined(__m68k__)
#define _MD_GET_SP(_th)		(_th)->md.context[2]
#elif defined(__i386__)


#define _MD_GET_SP(_th)    (((struct sigcontext *) (_th)->md.context)->sc_onstack)

#else
error Unknown NEXTSTEP architecture
#endif

#define PR_NUM_GCREGS	_JBLEN




#define _MD_INIT_CONTEXT(_thread, _sp, _main, status)  \
{  \
    *status = PR_TRUE;  \
    if (setjmp(CONTEXT(_thread))) {  \
        _main();  \
    }  \
    _MD_GET_SP(_thread) = (int) ((_sp) - 64); \
}

#define _MD_SWITCH_CONTEXT(_thread)  \
    if (!setjmp(CONTEXT(_thread))) {  \
	(_thread)->md.errcode = errno;  \
	_PR_Schedule();  \
    }




#define _MD_RESTORE_CONTEXT(_thread) \
{   \
    errno = (_thread)->md.errcode;  \
    _MD_SET_CURRENT_THREAD(_thread);  \
    longjmp(CONTEXT(_thread), 1);  \
}



struct _MDThread {
    PR_CONTEXT_TYPE context;
    int id;
    int errcode;
};

struct _MDThreadStack {
    PRInt8 notused;
};

struct _MDLock {
    PRInt8 notused;
};

struct _MDSemaphore {
    PRInt8 notused;
};

struct _MDCVar {
    PRInt8 notused;
};

struct _MDSegment {
    PRInt8 notused;
};




#define _PR_MD_MAX_OSFD FD_SETSIZE

struct _MDCPU_Unix {
    PRCList ioQ;
    PRUint32 ioq_timeout;
    PRInt32 ioq_max_osfd;
    PRInt32 ioq_osfd_cnt;
#ifndef _PR_USE_POLL
    fd_set fd_read_set, fd_write_set, fd_exception_set;
    PRInt16 fd_read_cnt[_PR_MD_MAX_OSFD],fd_write_cnt[_PR_MD_MAX_OSFD],
				fd_exception_cnt[_PR_MD_MAX_OSFD];
#else
	struct pollfd *ioq_pollfds;
	int ioq_pollfds_size;
#endif	
};

#define _PR_IOQ(_cpu)			((_cpu)->md.md_unix.ioQ)
#define _PR_ADD_TO_IOQ(_pq, _cpu) PR_APPEND_LINK(&_pq.links, &_PR_IOQ(_cpu))
#define _PR_FD_READ_SET(_cpu)		((_cpu)->md.md_unix.fd_read_set)
#define _PR_FD_READ_CNT(_cpu)		((_cpu)->md.md_unix.fd_read_cnt)
#define _PR_FD_WRITE_SET(_cpu)		((_cpu)->md.md_unix.fd_write_set)
#define _PR_FD_WRITE_CNT(_cpu)		((_cpu)->md.md_unix.fd_write_cnt)
#define _PR_FD_EXCEPTION_SET(_cpu)	((_cpu)->md.md_unix.fd_exception_set)
#define _PR_FD_EXCEPTION_CNT(_cpu)	((_cpu)->md.md_unix.fd_exception_cnt)
#define _PR_IOQ_TIMEOUT(_cpu)		((_cpu)->md.md_unix.ioq_timeout)
#define _PR_IOQ_MAX_OSFD(_cpu)		((_cpu)->md.md_unix.ioq_max_osfd)
#define _PR_IOQ_OSFD_CNT(_cpu)		((_cpu)->md.md_unix.ioq_osfd_cnt)
#define _PR_IOQ_POLLFDS(_cpu)		((_cpu)->md.md_unix.ioq_pollfds)
#define _PR_IOQ_POLLFDS_SIZE(_cpu)	((_cpu)->md.md_unix.ioq_pollfds_size)

#define _PR_IOQ_MIN_POLLFDS_SIZE(_cpu)	32

struct _MDCPU {
	struct _MDCPU_Unix md_unix;
};

#define _MD_INIT_LOCKS()
#define _MD_NEW_LOCK(lock) PR_SUCCESS
#define _MD_FREE_LOCK(lock)
#define _MD_LOCK(lock)
#define _MD_UNLOCK(lock)
#define _MD_INIT_IO()
#define _MD_IOQ_LOCK()
#define _MD_IOQ_UNLOCK()

extern PRStatus _MD_InitializeThread(PRThread *thread);

#define _MD_INIT_RUNNING_CPU(cpu)       _MD_unix_init_running_cpu(cpu)
#define _MD_INIT_THREAD                 _MD_InitializeThread
#define _MD_EXIT_THREAD(thread)
#define _MD_SUSPEND_THREAD(thread)      _MD_suspend_thread
#define _MD_RESUME_THREAD(thread)       _MD_resume_thread
#define _MD_CLEAN_THREAD(_thread)

extern PRStatus _MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize);
extern void _MD_SET_PRIORITY(struct _MDThread *thread, PRUintn newPri);
extern PRStatus _MD_WAIT(PRThread *, PRIntervalTime timeout);
extern PRStatus _MD_WAKEUP_WAITER(PRThread *);
extern void _MD_YIELD(void);

#endif 

extern void _MD_EarlyInit(void);
extern PRIntervalTime _PR_UNIX_GetInterval(void);
extern PRIntervalTime _PR_UNIX_TicksPerSecond(void);

#define _MD_EARLY_INIT                  _MD_EarlyInit
#define _MD_FINAL_INIT			_PR_UnixInit
#define _MD_GET_INTERVAL                  _PR_UNIX_GetInterval
#define _MD_INTERVAL_PER_SEC              _PR_UNIX_TicksPerSecond





#define _MD_SELECT(nfds,r,w,e,tv) syscall(SYS_select,nfds,r,w,e,tv)


#include <sys/uio.h>






#define SA_RESTART 0









caddr_t mmap(caddr_t addr, size_t len, int prot, int flags,
          int fildes, off_t off);
int munmap(caddr_t addr, size_t len);
int mprotect(caddr_t addr, size_t len, int prot);



caddr_t my_mmap(caddr_t addr, size_t len, int prot, int flags,
          int fildes, off_t off);
int my_munmap(caddr_t addr, size_t len);






char *strdup(const char *s1);







int brk(void *endds);
void *sbrk(int incr);

#endif 
