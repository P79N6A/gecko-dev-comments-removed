









#include "webrtc/base/asyncinvoker.h"

namespace rtc {

AsyncInvoker::AsyncInvoker() : destroying_(false) {}

AsyncInvoker::~AsyncInvoker() {
  destroying_ = true;
  SignalInvokerDestroyed();
  
  MessageQueueManager::Clear(this);
}

void AsyncInvoker::OnMessage(Message* msg) {
  
  ScopedRefMessageData<AsyncClosure>* data =
      static_cast<ScopedRefMessageData<AsyncClosure>*>(msg->pdata);
  scoped_refptr<AsyncClosure> closure = data->data();
  delete msg->pdata;
  msg->pdata = NULL;

  
  closure->Execute();
}

void AsyncInvoker::Flush(Thread* thread, uint32 id ) {
  if (destroying_) return;

  
  if (Thread::Current() != thread) {
    thread->Invoke<void>(Bind(&AsyncInvoker::Flush, this, thread, id));
    return;
  }

  MessageList removed;
  thread->Clear(this, id, &removed);
  for (MessageList::iterator it = removed.begin(); it != removed.end(); ++it) {
    
    thread->Send(it->phandler,
                 it->message_id,
                 it->pdata);
  }
}

void AsyncInvoker::DoInvoke(Thread* thread, AsyncClosure* closure,
                            uint32 id) {
  if (destroying_) {
    LOG(LS_WARNING) << "Tried to invoke while destroying the invoker.";
    
    delete closure;
    return;
  }
  thread->Post(this, id, new ScopedRefMessageData<AsyncClosure>(closure));
}

NotifyingAsyncClosureBase::NotifyingAsyncClosureBase(AsyncInvoker* invoker,
                                                     Thread* calling_thread)
    : invoker_(invoker), calling_thread_(calling_thread) {
  calling_thread->SignalQueueDestroyed.connect(
      this, &NotifyingAsyncClosureBase::CancelCallback);
  invoker->SignalInvokerDestroyed.connect(
      this, &NotifyingAsyncClosureBase::CancelCallback);
}

void NotifyingAsyncClosureBase::TriggerCallback() {
  CritScope cs(&crit_);
  if (!CallbackCanceled() && !callback_.empty()) {
    invoker_->AsyncInvoke<void>(calling_thread_, callback_);
  }
}

void NotifyingAsyncClosureBase::CancelCallback() {
  
  
  
  CritScope cs(&crit_);
  
  calling_thread_ = NULL;
}

}  
