



#include "base/waitable_event_watcher.h"

#include "base/condition_variable.h"
#include "base/lock.h"
#include "base/message_loop.h"
#include "base/waitable_event.h"

namespace base {

















class Flag : public RefCountedThreadSafe<Flag> {
 public:
  Flag() { flag_ = false; }

  void Set() {
    AutoLock locked(lock_);
    flag_ = true;
  }

  bool value() const {
    AutoLock locked(lock_);
    return flag_;
  }

 private:
  mutable Lock lock_;
  bool flag_;
};





class AsyncWaiter : public WaitableEvent::Waiter {
 public:
  AsyncWaiter(MessageLoop* message_loop, Task* task, Flag* flag)
      : message_loop_(message_loop),
        cb_task_(task),
        flag_(flag) { }

  bool Fire(WaitableEvent* event) {
    if (flag_->value()) {
      
      
      delete cb_task_;
    } else {
      message_loop_->PostTask(FROM_HERE, cb_task_);
    }

    
    
    delete this;

    
    
    return true;
  }

  
  bool Compare(void* tag) {
    return tag == flag_.get();
  }

 private:
  MessageLoop *const message_loop_;
  Task *const cb_task_;
  scoped_refptr<Flag> flag_;
};






class AsyncCallbackTask : public Task {
 public:
  AsyncCallbackTask(Flag* flag, WaitableEventWatcher::Delegate* delegate,
                    WaitableEvent* event)
      : flag_(flag),
        delegate_(delegate),
        event_(event) {
  }

  void Run() {
    
    if (!flag_->value()) {
      
      
      flag_->Set();
      delegate_->OnWaitableEventSignaled(event_);
    }

    
  }

 private:
  scoped_refptr<Flag> flag_;
  WaitableEventWatcher::Delegate *const delegate_;
  WaitableEvent *const event_;
};

WaitableEventWatcher::WaitableEventWatcher()
    : event_(NULL),
      message_loop_(NULL),
      cancel_flag_(NULL),
      callback_task_(NULL) {
}

WaitableEventWatcher::~WaitableEventWatcher() {
  StopWatching();
}





bool WaitableEventWatcher::StartWatching
    (WaitableEvent* event, WaitableEventWatcher::Delegate* delegate) {
  MessageLoop *const current_ml = MessageLoop::current();
  DCHECK(current_ml) << "Cannot create WaitableEventWatcher without a "
                        "current MessageLoop";

  
  
  
  if (cancel_flag_.get() && cancel_flag_->value()) {
    if (message_loop_) {
      message_loop_->RemoveDestructionObserver(this);
      message_loop_ = NULL;
    }

    cancel_flag_ = NULL;
  }

  DCHECK(!cancel_flag_.get()) << "StartWatching called while still watching";

  cancel_flag_ = new Flag;
  callback_task_ = new AsyncCallbackTask(cancel_flag_, delegate, event);
  WaitableEvent::WaitableEventKernel* kernel = event->kernel_.get();

  AutoLock locked(kernel->lock_);

  if (kernel->signaled_) {
    if (!kernel->manual_reset_)
      kernel->signaled_ = false;

    
    
    current_ml->PostTask(FROM_HERE, callback_task_);
    return true;
  }

  message_loop_ = current_ml;
  current_ml->AddDestructionObserver(this);

  event_ = event;
  kernel_ = kernel;
  waiter_ = new AsyncWaiter(current_ml, callback_task_, cancel_flag_);
  event->Enqueue(waiter_);

  return true;
}

void WaitableEventWatcher::StopWatching() {
  if (message_loop_) {
    message_loop_->RemoveDestructionObserver(this);
    message_loop_ = NULL;
  }

  if (!cancel_flag_.get())  
    return;

  if (cancel_flag_->value()) {
    
    
    cancel_flag_ = NULL;
    return;
  }

  if (!kernel_.get()) {
    
    
    
    
    
    
    
    
    cancel_flag_->Set();
    cancel_flag_ = NULL;
    return;
  }

  AutoLock locked(kernel_->lock_);
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  if (kernel_->Dequeue(waiter_, cancel_flag_.get())) {
    
    
    
    
    delete waiter_;
    delete callback_task_;
    cancel_flag_ = NULL;
    return;
  }

  
  
  
  
  cancel_flag_->Set();
  cancel_flag_ = NULL;

  
  
  
  
  
  
  
}

WaitableEvent* WaitableEventWatcher::GetWatchedEvent() {
  if (!cancel_flag_.get())
    return NULL;

  if (cancel_flag_->value())
    return NULL;

  return event_;
}






void WaitableEventWatcher::WillDestroyCurrentMessageLoop() {
  StopWatching();
}

}  
