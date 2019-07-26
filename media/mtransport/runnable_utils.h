







#ifndef runnable_utils_h__
#define runnable_utils_h__

#include "nsThreadUtils.h"
#include "mozilla/RefPtr.h"


namespace mozilla {

class runnable_args_base : public nsRunnable {
 public:
  NS_IMETHOD Run() = 0;
  virtual bool returns_value() const { return false; }
};
















#include "runnable_utils_generated.h"


static inline nsresult RUN_ON_THREAD(nsIEventTarget *thread, nsIRunnable *runnable, uint32_t flags) {
  RefPtr<nsIRunnable> runnable_ref(runnable);
  if (thread) {
    bool on;
    nsresult rv;
    rv = thread->IsOnCurrentThread(&on);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    NS_ENSURE_SUCCESS(rv, rv);
    if(!on) {
      return thread->Dispatch(runnable_ref, flags);
    }
  }
  return runnable_ref->Run();
}

static inline nsresult RUN_ON_THREAD(nsIEventTarget *thread, runnable_args_base *runnable, uint32_t flags) {
  
  
  
  MOZ_ASSERT((!(runnable->returns_value()) || (flags != NS_DISPATCH_NORMAL)));

  return RUN_ON_THREAD(thread, static_cast<nsIRunnable *>(runnable), flags);
}

#ifdef MOZ_DEBUG
#define ASSERT_ON_THREAD(t) do {                \
    if (t) {                                    \
      bool on;                                    \
      nsresult rv;                                \
      rv = t->IsOnCurrentThread(&on);             \
      MOZ_ASSERT(NS_SUCCEEDED(rv));               \
      MOZ_ASSERT(on);                             \
    }                                           \
  } while(0)
#else
#define ASSERT_ON_THREAD(t)
#endif

} 

#endif
