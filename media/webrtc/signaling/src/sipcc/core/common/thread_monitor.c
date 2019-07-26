



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "thread_monitor.h"
#include "mozilla/Assertions.h"




static cprThread_t thread_list[THREADMON_MAX];
static boolean wait_list[THREADMON_MAX];









void thread_started(thread_monitor_id_t monitor_id, cprThread_t thread) {
  MOZ_ASSERT(monitor_id < THREADMON_MAX);
  if (monitor_id >= THREADMON_MAX) {
    return;
  }

  
  MOZ_ASSERT(thread_list[monitor_id] == NULL);

  thread_list[monitor_id] = thread;
  wait_list[monitor_id] = TRUE;
}































static thread_ended_dispatcher_funct dispatcher = NULL;
static join_wait_funct waiter = NULL;

void init_thread_monitor(thread_ended_dispatcher_funct dispatch,
                         join_wait_funct wait) {
  dispatcher = dispatch;
  waiter = wait;
}

static void thread_ended_m(thread_monitor_id_t monitor_id) {
  MOZ_ASSERT(dispatcher);
  MOZ_ASSERT(waiter);
  MOZ_ASSERT(monitor_id < THREADMON_MAX);
  if (monitor_id >= THREADMON_MAX) {
    return;
  }

  
  MOZ_ASSERT(thread_list[monitor_id]);
  MOZ_ASSERT(wait_list[monitor_id]);

  wait_list[monitor_id] = FALSE;
}

void thread_ended(thread_monitor_id_t monitor_id) {
  MOZ_ASSERT(dispatcher);
  dispatcher (&thread_ended_m, monitor_id);
}






void join_all_threads() {
  int i;
  MOZ_ASSERT(dispatcher);
  MOZ_ASSERT(waiter);

  for (i = 0; i < THREADMON_MAX; i++) {
    if (thread_list[i] != NULL) {
      while (wait_list[i]) {
        waiter();
      }
      cprJoinThread(thread_list[i]);
      cpr_free(thread_list[i]);
      thread_list[i] = NULL;
    }
  }
}
