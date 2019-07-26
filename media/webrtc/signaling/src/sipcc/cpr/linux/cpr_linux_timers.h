






































#ifndef _CPR_LINUX_TIMERS_H_
#define _CPR_LINUX_TIMERS_H_
#include <pthread.h>











#define timerGranularity 10



typedef struct cpr_timer_s
{
  const char *name;
  uint32_t cprTimerId;
  cprMsgQueue_t callBackMsgQueue;
  uint16_t applicationTimerId;
  uint16_t applicationMsgId;
  void *data;
  union {
    void *handlePtr;
  }u;
}cpr_timer_t;


typedef struct timerDef
{
    int32_t duration;
    boolean timerActive;
    cpr_timer_t *cprTimerPtr;
    struct timerDef *previous;
    struct timerDef *next;
} timerBlk;


extern pthread_mutex_t timerMutex;


extern void *linuxTimerTick(void *);

cprRC_t cpr_timer_pre_init(void);
cprRC_t cpr_timer_de_init(void);

#endif
