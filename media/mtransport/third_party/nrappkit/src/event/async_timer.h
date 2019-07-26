






































#ifndef _async_timer_h
#define _async_timer_h


int NR_async_timer_init(void);
int NR_async_timer_set(int delay_ms,NR_async_cb cb,void *cb_arg,char *function,int line,void **handle);
int NR_async_timer_cancel(void *handle);
int NR_async_timer_update_time(struct timeval *tv);
int NR_async_timer_next_timeout(int *delay_ms);
int NR_async_timer_sanity_check_for_cb_deleted(NR_async_cb cb,void *cb_arg);

#define NR_ASYNC_TIMER_SET(d,c,a,hp) NR_async_timer_set(d,c,a,(char *)__FUNCTION__,__LINE__,hp)

#endif

