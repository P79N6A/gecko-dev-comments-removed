





























#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#endif

#ifndef _NETINET_SCTP_CALLOUT_
#define _NETINET_SCTP_CALLOUT_











#define _SCTP_NEEDS_CALLOUT_ 1

#define SCTP_TICKS_PER_FASTTIMO 20	/* called about every 20ms */

#if defined(__Userspace__)
#if defined(__Userspace_os_Windows)
#define SCTP_TIMERQ_LOCK()          EnterCriticalSection(&SCTP_BASE_VAR(timer_mtx))
#define SCTP_TIMERQ_UNLOCK()        LeaveCriticalSection(&SCTP_BASE_VAR(timer_mtx))
#define SCTP_TIMERQ_LOCK_INIT()     InitializeCriticalSection(&SCTP_BASE_VAR(timer_mtx))
#define SCTP_TIMERQ_LOCK_DESTROY()  DeleteCriticalSection(&SCTP_BASE_VAR(timer_mtx))
#else
#define SCTP_TIMERQ_LOCK()          (void)pthread_mutex_lock(&SCTP_BASE_VAR(timer_mtx))
#define SCTP_TIMERQ_UNLOCK()        (void)pthread_mutex_unlock(&SCTP_BASE_VAR(timer_mtx))
#define SCTP_TIMERQ_LOCK_INIT()     (void)pthread_mutex_init(&SCTP_BASE_VAR(timer_mtx), NULL)
#define SCTP_TIMERQ_LOCK_DESTROY()  (void)pthread_mutex_destroy(&SCTP_BASE_VAR(timer_mtx))
#endif

extern int ticks;
extern void sctp_start_timer();
#endif

TAILQ_HEAD(calloutlist, sctp_callout);

struct sctp_callout {
	TAILQ_ENTRY(sctp_callout) tqe;
	int c_time;		
	void *c_arg;		
	void (*c_func)(void *);	
	int c_flags;		
};
typedef struct sctp_callout sctp_os_timer_t;

#define	SCTP_CALLOUT_ACTIVE	0x0002	/* callout is currently active */
#define	SCTP_CALLOUT_PENDING	0x0004	/* callout is waiting for timeout */

void sctp_os_timer_init(sctp_os_timer_t *tmr);
void sctp_os_timer_start(sctp_os_timer_t *, int, void (*)(void *), void *);
int sctp_os_timer_stop(sctp_os_timer_t *);

#define SCTP_OS_TIMER_INIT	sctp_os_timer_init
#define SCTP_OS_TIMER_START	sctp_os_timer_start
#define SCTP_OS_TIMER_STOP	sctp_os_timer_stop

#define SCTP_OS_TIMER_STOP_DRAIN SCTP_OS_TIMER_STOP
#define	SCTP_OS_TIMER_PENDING(tmr) ((tmr)->c_flags & SCTP_CALLOUT_PENDING)
#define	SCTP_OS_TIMER_ACTIVE(tmr) ((tmr)->c_flags & SCTP_CALLOUT_ACTIVE)
#define	SCTP_OS_TIMER_DEACTIVATE(tmr) ((tmr)->c_flags &= ~SCTP_CALLOUT_ACTIVE)

void sctp_timeout(void *);

#endif
