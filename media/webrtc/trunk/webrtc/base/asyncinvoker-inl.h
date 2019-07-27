









#ifndef WEBRTC_BASE_ASYNCINVOKER_INL_H_
#define WEBRTC_BASE_ASYNCINVOKER_INL_H_

#include "webrtc/base/bind.h"
#include "webrtc/base/callback.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/thread.h"

namespace rtc {

class AsyncInvoker;




class AsyncClosure : public RefCountInterface {
 public:
  virtual ~AsyncClosure() {}
  
  
  virtual void Execute() = 0;
};


template <class FunctorT>
class FireAndForgetAsyncClosure : public AsyncClosure {
 public:
  explicit FireAndForgetAsyncClosure(const FunctorT& functor)
      : functor_(functor) {}
  virtual void Execute() {
    functor_();
  }
 private:
  FunctorT functor_;
};




class NotifyingAsyncClosureBase : public AsyncClosure,
                                  public sigslot::has_slots<> {
 public:
  virtual ~NotifyingAsyncClosureBase() { disconnect_all(); }

 protected:
  NotifyingAsyncClosureBase(AsyncInvoker* invoker, Thread* calling_thread);
  void TriggerCallback();
  void SetCallback(const Callback0<void>& callback) {
    CritScope cs(&crit_);
    callback_ = callback;
  }
  bool CallbackCanceled() const { return calling_thread_ == NULL; }

 private:
  Callback0<void> callback_;
  CriticalSection crit_;
  AsyncInvoker* invoker_;
  Thread* calling_thread_;

  void CancelCallback();
};


template <class ReturnT, class FunctorT, class HostT>
class NotifyingAsyncClosure : public NotifyingAsyncClosureBase {
 public:
  NotifyingAsyncClosure(AsyncInvoker* invoker,
                        Thread* calling_thread,
                        const FunctorT& functor,
                        void (HostT::*callback)(ReturnT),
                        HostT* callback_host)
      :  NotifyingAsyncClosureBase(invoker, calling_thread),
         functor_(functor),
         callback_(callback),
         callback_host_(callback_host) {}
  virtual void Execute() {
    ReturnT result = functor_();
    if (!CallbackCanceled()) {
      SetCallback(Callback0<void>(Bind(callback_, callback_host_, result)));
      TriggerCallback();
    }
  }

 private:
  FunctorT functor_;
  void (HostT::*callback_)(ReturnT);
  HostT* callback_host_;
};


template <class FunctorT, class HostT>
class NotifyingAsyncClosure<void, FunctorT, HostT>
    : public NotifyingAsyncClosureBase {
 public:
  NotifyingAsyncClosure(AsyncInvoker* invoker,
                        Thread* calling_thread,
                        const FunctorT& functor,
                        void (HostT::*callback)(),
                        HostT* callback_host)
      : NotifyingAsyncClosureBase(invoker, calling_thread),
        functor_(functor) {
    SetCallback(Callback0<void>(Bind(callback, callback_host)));
  }
  virtual void Execute() {
    functor_();
    TriggerCallback();
  }

 private:
  FunctorT functor_;
};

}  

#endif  
