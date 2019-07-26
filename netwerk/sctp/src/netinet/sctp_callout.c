































#include <netinet/sctp_os.h>
#include <netinet/sctp_callout.h>
#include <netinet/sctp_pcb.h>




#if defined(__APPLE__) || defined(__Userspace__)
int ticks = 0;
#else
extern int ticks;
#endif







static sctp_os_timer_t *sctp_os_timer_current = NULL;
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

#if defined(__APPLE__)




#endif
void
sctp_timeout(void *arg SCTP_UNUSED)
{
	sctp_os_timer_t *c;
	void (*c_func)(void *);
	void *c_arg;

	SCTP_TIMERQ_LOCK();
#if defined(__APPLE__)
	
	ticks += SCTP_BASE_VAR(sctp_main_timer_ticks);
#endif
	c = TAILQ_FIRST(&SCTP_BASE_INFO(callqueue));
	while (c) {
		if (c->c_time <= ticks) {
			sctp_os_timer_next = TAILQ_NEXT(c, tqe);
			TAILQ_REMOVE(&SCTP_BASE_INFO(callqueue), c, tqe);
			c_func = c->c_func;
			c_arg = c->c_arg;
			c->c_flags &= ~SCTP_CALLOUT_PENDING;
			sctp_os_timer_current = c;
			SCTP_TIMERQ_UNLOCK();
			c_func(c_arg);
			SCTP_TIMERQ_LOCK();
			sctp_os_timer_current = NULL;
			c = sctp_os_timer_next;
		} else {
			c = TAILQ_NEXT(c, tqe);
		}
	}
	sctp_os_timer_next = NULL;
	SCTP_TIMERQ_UNLOCK();

#if defined(__APPLE__)
	
	sctp_start_main_timer();
#endif
}
