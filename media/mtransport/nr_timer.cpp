


















































#include <string>

#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIEventTarget.h"
#include "nsITimer.h"
#include "nsNetCID.h"
#include "runnable_utils.h"

extern "C" {
#include "nr_api.h"
#include "async_timer.h"
}


namespace mozilla {

class nrappkitTimerCallback : public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  nrappkitTimerCallback(NR_async_cb cb, void *cb_arg,
                        const char *function, int line)
    : cb_(cb), cb_arg_(cb_arg), function_(function), line_(line) {
  }

private:
  virtual ~nrappkitTimerCallback() {}

protected:
  
  NR_async_cb cb_;
  void *cb_arg_;
  std::string function_;
  int line_;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nrappkitTimerCallback, nsITimerCallback)

NS_IMETHODIMP nrappkitTimerCallback::Notify(nsITimer *timer) {
  r_log(LOG_GENERIC, LOG_DEBUG, "Timer callback fired (set in %s:%d)",
        function_.c_str(), line_);

  cb_(0, 0, cb_arg_);

  
  timer->Release();
  return NS_OK;
}
}  


using namespace mozilla;



static void CheckSTSThread() {
  nsresult rv;

  nsCOMPtr<nsIEventTarget> sts_thread;

  sts_thread = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);

  MOZ_ASSERT(NS_SUCCEEDED(rv));
  ASSERT_ON_THREAD(sts_thread);
}

int NR_async_timer_set(int timeout, NR_async_cb cb, void *arg, char *func,
                       int l, void **handle) {
  nsresult rv;
  CheckSTSThread();

  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return(R_FAILED);
  }

  rv = timer->InitWithCallback(new nrappkitTimerCallback(cb, arg, func, l),
                               timeout, nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv)) {
    return R_FAILED;
  }

  
  timer->AddRef();

  if (handle)
    *handle = timer.get();
  
  
  

  return 0;
}

int NR_async_schedule(NR_async_cb cb, void *arg, char *func, int l) {
  
  
  return NR_async_timer_set(0, cb, arg, func, l, nullptr);
}

int NR_async_timer_cancel(void *handle) {
  CheckSTSThread();

  if (!handle)
    return 0;

  nsITimer *timer = static_cast<nsITimer *>(handle);

  timer->Cancel();
  
  timer->Release();

  return 0;
}

