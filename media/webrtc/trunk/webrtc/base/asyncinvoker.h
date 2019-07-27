









#ifndef WEBRTC_BASE_ASYNCINVOKER_H_
#define WEBRTC_BASE_ASYNCINVOKER_H_

#include "webrtc/base/asyncinvoker-inl.h"
#include "webrtc/base/bind.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/scopedptrcollection.h"
#include "webrtc/base/thread.h"

namespace rtc {
















































class AsyncInvoker : public MessageHandler {
 public:
  AsyncInvoker();
  virtual ~AsyncInvoker();

  
  
  template <class ReturnT, class FunctorT>
  void AsyncInvoke(Thread* thread,
                   const FunctorT& functor,
                   uint32 id = 0) {
    AsyncClosure* closure =
        new RefCountedObject<FireAndForgetAsyncClosure<FunctorT> >(functor);
    DoInvoke(thread, closure, id);
  }

  
  template <class ReturnT, class FunctorT, class HostT>
  void AsyncInvoke(Thread* thread,
                   const FunctorT& functor,
                   void (HostT::*callback)(ReturnT),
                   HostT* callback_host,
                   uint32 id = 0) {
    AsyncClosure* closure =
        new RefCountedObject<NotifyingAsyncClosure<ReturnT, FunctorT, HostT> >(
            this, Thread::Current(), functor, callback, callback_host);
    DoInvoke(thread, closure, id);
  }

  
  
  template <class ReturnT, class FunctorT, class HostT>
  void AsyncInvoke(Thread* thread,
                   const FunctorT& functor,
                   void (HostT::*callback)(),
                   HostT* callback_host,
                   uint32 id = 0) {
    AsyncClosure* closure =
        new RefCountedObject<NotifyingAsyncClosure<void, FunctorT, HostT> >(
            this, Thread::Current(), functor, callback, callback_host);
    DoInvoke(thread, closure, id);
  }

  
  
  
  
  
  void Flush(Thread* thread, uint32 id = MQID_ANY);

  
  sigslot::signal0<> SignalInvokerDestroyed;

 private:
  virtual void OnMessage(Message* msg);
  void DoInvoke(Thread* thread, AsyncClosure* closure, uint32 id);

  bool destroying_;

  DISALLOW_COPY_AND_ASSIGN(AsyncInvoker);
};

}  


#endif  
