































#if defined(__Userspace__)
#include <sys/types.h>
#if !defined (__Userspace_os_Windows)
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#endif
#if defined(__Userspace_os_NaCl)
#include <sys/select.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/sctp_sysctl.h>
#include <netinet/sctp_pcb.h>
#else
#include <netinet/sctp_os.h>
#include <netinet/sctp_callout.h>
#include <netinet/sctp_pcb.h>
#endif




#if defined(__APPLE__) || defined(__Userspace__)
int ticks = 0;
#else
extern int ticks;
#endif






static sctp_os_timer_t *sctp_os_timer_next = NULL;

void
sctp_os_timer_init(sctp_os_timer_t *c)
{
	bzero(c, sizeof(*c));
}

void
sctp_os_timer_start(sctp_os_timer_t *c, int to_ticks, void (*ftn) (void *),
                    void *arg)
{
	
	if ((c == NULL) || (ftn == NULL))
	    return;

	SCTP_TIMERQ_LOCK();
	
	if (c->c_flags & SCTP_CALLOUT_PENDING) {
		if (c == sctp_os_timer_next) {
			sctp_os_timer_next = TAILQ_NEXT(c, tqe);
		}
		TAILQ_REMOVE(&SCTP_BASE_INFO(callqueue), c, tqe);
		





	}

	



	if (to_ticks <= 0)
		to_ticks = 1;

	c->c_arg = arg;
	c->c_flags = (SCTP_CALLOUT_ACTIVE | SCTP_CALLOUT_PENDING);
	c->c_func = ftn;
	c->c_time = ticks + to_ticks;
	TAILQ_INSERT_TAIL(&SCTP_BASE_INFO(callqueue), c, tqe);
	SCTP_TIMERQ_UNLOCK();
}

int
sctp_os_timer_stop(sctp_os_timer_t *c)
{
	SCTP_TIMERQ_LOCK();
	


	if (!(c->c_flags & SCTP_CALLOUT_PENDING)) {
		c->c_flags &= ~SCTP_CALLOUT_ACTIVE;
		SCTP_TIMERQ_UNLOCK();
		return (0);
	}
	c->c_flags &= ~(SCTP_CALLOUT_ACTIVE | SCTP_CALLOUT_PENDING);
	if (c == sctp_os_timer_next) {
		sctp_os_timer_next = TAILQ_NEXT(c, tqe);
	}
	TAILQ_REMOVE(&SCTP_BASE_INFO(callqueue), c, tqe);
	SCTP_TIMERQ_UNLOCK();
	return (1);
}

static void
sctp_handle_tick(int delta)
{
	sctp_os_timer_t *c;
	void (*c_func)(void *);
	void *c_arg;

	SCTP_TIMERQ_LOCK();
	
	ticks += delta;
	c = TAILQ_FIRST(&SCTP_BASE_INFO(callqueue));
	while (c) {
		if (c->c_time <= ticks) {
			sctp_os_timer_next = TAILQ_NEXT(c, tqe);
			TAILQ_REMOVE(&SCTP_BASE_INFO(callqueue), c, tqe);
			c_func = c->c_func;
			c_arg = c->c_arg;
			c->c_flags &= ~SCTP_CALLOUT_PENDING;
			SCTP_TIMERQ_UNLOCK();
			c_func(c_arg);
			SCTP_TIMERQ_LOCK();
			c = sctp_os_timer_next;
		} else {
			c = TAILQ_NEXT(c, tqe);
		}
	}
	sctp_os_timer_next = NULL;
	SCTP_TIMERQ_UNLOCK();
}

#if defined(__APPLE__)
void
sctp_timeout(void *arg SCTP_UNUSED)
{
	sctp_handle_tick(SCTP_BASE_VAR(sctp_main_timer_ticks));
	sctp_start_main_timer();
}
#endif

#if defined(__Userspace__)
#define TIMEOUT_INTERVAL 10

void *
user_sctp_timer_iterate(void *arg)
{
	for (;;) {
#if defined (__Userspace_os_Windows)
		Sleep(TIMEOUT_INTERVAL);
#else
		struct timeval timeout;

		timeout.tv_sec  = 0;
		timeout.tv_usec = 1000 * TIMEOUT_INTERVAL;
		select(0, NULL, NULL, NULL, &timeout);
#endif
		if (SCTP_BASE_VAR(timer_thread_should_exit)) {
			break;
		}
		sctp_handle_tick(MSEC_TO_TICKS(TIMEOUT_INTERVAL));
	}
	return (NULL);
}

void
sctp_start_timer(void)
{
	



#if defined (__Userspace_os_Windows)
	if ((SCTP_BASE_VAR(timer_thread) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)user_sctp_timer_iterate, NULL, 0, NULL)) == NULL) {
		SCTP_PRINTF("ERROR; Creating ithread failed\n");
	}
#else
	int rc;

	rc = pthread_create(&SCTP_BASE_VAR(timer_thread), NULL, user_sctp_timer_iterate, NULL);
	if (rc) {
		SCTP_PRINTF("ERROR; return code from pthread_create() is %d\n", rc);
	}
#endif
}

#endif
