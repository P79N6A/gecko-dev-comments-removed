






































#ifndef _async_wait_h
#define _async_wait_h

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <csi_platform.h>

typedef void (*NR_async_cb)(NR_SOCKET resource,int how,void *arg);

#define NR_ASYNC_WAIT_READ 0
#define NR_ASYNC_WAIT_WRITE 1


int NR_async_wait_init(void);
int NR_async_wait(NR_SOCKET sock, int how, NR_async_cb cb,void *cb_arg,
                            char *function,int line);
int NR_async_cancel(NR_SOCKET sock,int how);
int NR_async_schedule(NR_async_cb cb,void *arg,char *function,int line);
int NR_async_event_wait(int *eventsp);
int NR_async_event_wait2(int *eventsp,struct timeval *tv);


#ifdef NR_DEBUG_ASYNC
#define NR_ASYNC_WAIT(s,h,cb,arg) do { \
fprintf(stderr,"NR_async_wait(%d,%s,%s) at %s(%d)\n",s,#h,#cb,__FUNCTION__,__LINE__); \
       NR_async_wait(s,h,cb,arg,(char *)__FUNCTION__,__LINE__);            \
} while (0)
#define NR_ASYNC_SCHEDULE(cb,arg) do { \
fprintf(stderr,"NR_async_schedule(%s) at %s(%d)\n",#cb,__FUNCTION__,__LINE__);\
       NR_async_schedule(cb,arg,(char *)__FUNCTION__,__LINE__);            \
} while (0)
#define NR_ASYNC_CANCEL(s,h) do { \
       fprintf(stderr,"NR_async_cancel(%d,%s) at %s(%d)\n",s,#h,(char *)__FUNCTION__,__LINE__); \
NR_async_cancel(s,h); \
} while (0)
#else
#define NR_ASYNC_WAIT(a,b,c,d) NR_async_wait(a,b,c,d,(char *)__FUNCTION__,__LINE__)
#define NR_ASYNC_SCHEDULE(a,b) NR_async_schedule(a,b,(char *)__FUNCTION__,__LINE__)
#define NR_ASYNC_CANCEL NR_async_cancel
#endif

#endif

