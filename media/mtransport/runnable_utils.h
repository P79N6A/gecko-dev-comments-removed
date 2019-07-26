







#ifndef runnable_utils_h__
#define runnable_utils_h__

#include "nsThreadUtils.h"
#include "mozilla/RefPtr.h"


namespace mozilla {

class runnable_args_base : public nsRunnable {
 public:
  NS_IMETHOD Run() = 0;
};

















#include "runnable_utils_generated.h"


static inline nsresult RUN_ON_THREAD(nsIEventTarget *thread, nsIRunnable *runnable, uint32_t flags) {
  RefPtr<nsIRunnable> runnable_ref(runnable);
  
  if (thread && (thread != nsRefPtr<nsIThread>(do_GetCurrentThread()))) {
    return thread->Dispatch(runnable_ref, flags);
  }

  return runnable_ref->Run();
}

#define ASSERT_ON_THREAD(t) do {                \
    if (t) {                                    \
      bool on;                                    \
      nsresult rv;                                \
      rv = t->IsOnCurrentThread(&on);             \
      MOZ_ASSERT(NS_SUCCEEDED(rv));               \
      MOZ_ASSERT(on);                             \
    }                                           \
  } while(0)
}

#endif

