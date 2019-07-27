


















































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

class nrappkitCallback  {
 public:
  nrappkitCallback(NR_async_cb cb, void *cb_arg,
                   const char *function, int line)
    : cb_(cb), cb_arg_(cb_arg), function_(function), line_(line) {
  }
  virtual ~nrappkitCallback() {}

  virtual void Cancel() = 0;

protected:
  
  NR_async_cb cb_;
  void *cb_arg_;
  std::string function_;
  int line_;
};

class nrappkitTimerCallback : public nrappkitCallback,
                              public nsITimerCallback {
 public:
  
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  nrappkitTimerCallback(NR_async_cb cb, void *cb_arg,
                        const char *function, int line,
                        nsITimer *timer)
      : nrappkitCallback(cb, cb_arg, function, line),
        timer_(timer) {}

  virtual void Cancel() override {
    AddRef();  
               
    timer_->Cancel();
    timer_->Release();
    Release(); 
  }

 private:
  nsITimer* timer_;
  virtual ~nrappkitTimerCallback() {}
};

NS_IMPL_ISUPPORTS(nrappkitTimerCallback, nsITimerCallback)

NS_IMETHODIMP nrappkitTimerCallback::Notify(nsITimer *timer) {
  r_log(LOG_GENERIC, LOG_DEBUG, "Timer callback fired (set in %s:%d)",
        function_.c_str(), line_);
  MOZ_ASSERT(timer == timer_);
  cb_(0, 0, cb_arg_);

  
  timer->Release();
  return NS_OK;
}

class nrappkitScheduledCallback : public nrappkitCallback {
 public:

  nrappkitScheduledCallback(NR_async_cb cb, void *cb_arg,
                            const char *function, int line)
      : nrappkitCallback(cb, cb_arg, function, line) {}

  void Run() {
    if (cb_) {
      cb_(0, 0, cb_arg_);
    }
  }

  virtual void Cancel() override {
    cb_ = nullptr;
  }

  ~nrappkitScheduledCallback() {}
};

}  


using namespace mozilla;

static nsCOMPtr<nsIEventTarget> GetSTSThread() {
  nsresult rv;

  nsCOMPtr<nsIEventTarget> sts_thread;

  sts_thread = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  return sts_thread;
}



static void CheckSTSThread() {
  nsCOMPtr<nsIEventTarget> sts_thread = GetSTSThread();

  ASSERT_ON_THREAD(sts_thread);
}

static int nr_async_timer_set_zero(NR_async_cb cb, void *arg,
                                   char *func, int l,
                                   nrappkitCallback **handle) {
  nrappkitScheduledCallback* callback(new nrappkitScheduledCallback(
      cb, arg, func, l));

  nsresult rv = GetSTSThread()->Dispatch(WrapRunnable(
      nsAutoPtr<nrappkitScheduledCallback>(callback),
      &nrappkitScheduledCallback::Run),
                        NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv))
    return R_FAILED;

  *handle = callback;

  
  
  
  return 0;
}

static int nr_async_timer_set_nonzero(int timeout, NR_async_cb cb, void *arg,
                                      char *func, int l,
                                      nrappkitCallback **handle) {
  nsresult rv;
  CheckSTSThread();

  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return(R_FAILED);
  }

  nrappkitTimerCallback* callback =
      new nrappkitTimerCallback(cb, arg, func, l, timer);
  rv = timer->InitWithCallback(callback, timeout, nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv)) {
    return R_FAILED;
  }

  
  timer->AddRef();

  *handle = callback;

  return 0;
}

int NR_async_timer_set(int timeout, NR_async_cb cb, void *arg,
                       char *func, int l, void **handle) {
  CheckSTSThread();

  nrappkitCallback *callback;
  int r;

  if (!timeout) {
    r = nr_async_timer_set_zero(cb, arg, func, l, &callback);
  } else {
    r = nr_async_timer_set_nonzero(timeout, cb, arg, func, l, &callback);
  }

  if (r)
    return r;

  if (handle)
    *handle = callback;

  return 0;
}

int NR_async_schedule(NR_async_cb cb, void *arg, char *func, int l) {
  
  
  return NR_async_timer_set(0, cb, arg, func, l, nullptr);
}

int NR_async_timer_cancel(void *handle) {
  
  
  
  if (!handle)
    return 0;

  CheckSTSThread();

  nrappkitCallback* callback = static_cast<nrappkitCallback *>(handle);
  callback->Cancel();

  return 0;
}

