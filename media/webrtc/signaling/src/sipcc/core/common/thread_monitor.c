



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "thread_monitor.h"
#include "prtypes.h"
#include "mozilla/Assertions.h"




static cprThread_t thread_list[THREADMON_MAX];









void thread_started(thread_monitor_id_t monitor_id, cprThread_t thread) {
  MOZ_ASSERT(monitor_id < THREADMON_MAX);
  if (monitor_id >= THREADMON_MAX) {
    return;
  }

  
  MOZ_ASSERT(thread_list[monitor_id] == NULL);

  thread_list[monitor_id] = thread;
}






void join_all_threads() {
  int i;

  for (i = 0; i < THREADMON_MAX; i++) {
    if (thread_list[i] != NULL) {
      cprJoinThread(thread_list[i]);
      cpr_free(thread_list[i]);
      thread_list[i] = NULL;
    }
  }
}
