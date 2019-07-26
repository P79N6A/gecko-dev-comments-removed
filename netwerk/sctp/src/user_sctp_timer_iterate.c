


























#include <sys/types.h>
#if !defined (__Userspace_os_Windows)
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_sysctl.h>
#include "netinet/sctp_callout.h"




#define TIMEOUT_INTERVAL 10

extern int ticks;

void *
user_sctp_timer_iterate(void *arg)
{
	sctp_os_timer_t *c;
	void (*c_func)(void *);
	void *c_arg;
	sctp_os_timer_t *sctp_os_timer_next;
	






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
		SCTP_TIMERQ_LOCK();
		
		ticks += MSEC_TO_TICKS(TIMEOUT_INTERVAL);
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
		SCTP_TIMERQ_UNLOCK();
	}
#if defined (__Userspace_os_Windows)
	ExitThread(0);
#else
	pthread_exit(NULL);
#endif
	return NULL;
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
