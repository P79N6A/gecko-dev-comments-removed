



#ifndef THREAD_MONITOR_H_
#define THREAD_MONITOR_H_

#include "cpr_threads.h"





typedef enum {
  THREADMON_CCAPP,
  THREADMON_SIP,
  THREADMON_MSGQ,
  THREADMON_GSM,
  THREADMON_MAX
} thread_monitor_id_t;









void thread_started(thread_monitor_id_t monitor_id, cprThread_t thread);










void thread_ended(thread_monitor_id_t monitor_id);

typedef void (*thread_ended_funct)(thread_monitor_id_t);
typedef void (*thread_ended_dispatcher_funct)(thread_ended_funct func, thread_monitor_id_t);
typedef void (*join_wait_funct)();




void init_thread_monitor(thread_ended_dispatcher_funct dispatch, join_wait_funct wait);






void join_all_threads();

#endif

