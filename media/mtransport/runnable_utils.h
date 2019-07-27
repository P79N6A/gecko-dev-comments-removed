







#ifndef runnable_utils_h__
#define runnable_utils_h__

#include "nsThreadUtils.h"
#include "mozilla/RefPtr.h"


namespace mozilla {

namespace detail {

enum RunnableResult {
  NoResult,
  ReturnsResult
};

static inline nsresult
RunOnThreadInternal(nsIEventTarget *thread, nsIRunnable *runnable, uint32_t flags)
{
  nsCOMPtr<nsIRunnable> runnable_ref(runnable);
  if (thread) {
    bool on;
    nsresult rv;
    rv = thread->IsOnCurrentThread(&on);

    
    if (rv != NS_ERROR_NOT_INITIALIZED) {
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    }

    NS_ENSURE_SUCCESS(rv, rv);
    if (!on) {
      return thread->Dispatch(runnable_ref, flags);
    }
  }
  return runnable_ref->Run();
}

template<RunnableResult result>
class runnable_args_base : public nsRunnable {
 public:
  NS_IMETHOD Run() = 0;
};

}
















#include "runnable_utils_generated.h"

static inline nsresult RUN_ON_THREAD(nsIEventTarget *thread, detail::runnable_args_base<detail::NoResult> *runnable, uint32_t flags) {
  return detail::RunOnThreadInternal(thread, static_cast<nsIRunnable *>(runnable), flags);
}

static inline nsresult
RUN_ON_THREAD(nsIEventTarget *thread, detail::runnable_args_base<detail::ReturnsResult> *runnable)
{
  return detail::RunOnThreadInternal(thread, static_cast<nsIRunnable *>(runnable), NS_DISPATCH_SYNC);
}

#ifdef DEBUG
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
