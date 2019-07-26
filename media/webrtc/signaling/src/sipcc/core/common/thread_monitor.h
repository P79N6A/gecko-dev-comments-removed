



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






void join_all_threads();

#endif

