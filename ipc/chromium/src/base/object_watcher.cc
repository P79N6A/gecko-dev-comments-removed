



#include "base/object_watcher.h"

#include "base/logging.h"

namespace base {



struct ObjectWatcher::Watch : public Task {
  ObjectWatcher* watcher;    
  HANDLE object;             
  HANDLE wait_object;        
  MessageLoop* origin_loop;  
  Delegate* delegate;        
  bool did_signal;           

  virtual void Run() {
    
    
    if (!watcher)
      return;

    DCHECK(did_signal);
    watcher->StopWatching();

    delegate->OnObjectSignaled(object);
  }
};



ObjectWatcher::ObjectWatcher() : watch_(NULL) {
}

ObjectWatcher::~ObjectWatcher() {
  StopWatching();
}

bool ObjectWatcher::StartWatching(HANDLE object, Delegate* delegate) {
  if (watch_) {
    NOTREACHED() << "Already watching an object";
    return false;
  }

  Watch* watch = new Watch;
  watch->watcher = this;
  watch->object = object;
  watch->origin_loop = MessageLoop::current();
  watch->delegate = delegate;
  watch->did_signal = false;

  
  
  DWORD wait_flags = WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE;

  if (!RegisterWaitForSingleObject(&watch->wait_object, object, DoneWaiting,
                                   watch, INFINITE, wait_flags)) {
    NOTREACHED() << "RegisterWaitForSingleObject failed: " << GetLastError();
    delete watch;
    return false;
  }

  watch_ = watch;

  
  
  MessageLoop::current()->AddDestructionObserver(this);
  return true;
}

bool ObjectWatcher::StopWatching() {
  if (!watch_)
    return false;

  
  DCHECK(watch_->origin_loop == MessageLoop::current());

  
  
  if (!UnregisterWaitEx(watch_->wait_object, INVALID_HANDLE_VALUE)) {
    NOTREACHED() << "UnregisterWaitEx failed: " << GetLastError();
    return false;
  }

  
  
  
  MemoryBarrier();

  
  
  watch_->watcher = NULL;

  
  
  
  if (!watch_->did_signal)
    delete watch_;

  watch_ = NULL;

  MessageLoop::current()->RemoveDestructionObserver(this);
  return true;
}

HANDLE ObjectWatcher::GetWatchedObject() {
  if (!watch_)
    return NULL;

  return watch_->object;
}


void CALLBACK ObjectWatcher::DoneWaiting(void* param, BOOLEAN timed_out) {
  DCHECK(!timed_out);

  Watch* watch = static_cast<Watch*>(param);

  
  watch->did_signal = true;

  
  
  
  watch->origin_loop->PostTask(FROM_HERE, watch);
}

void ObjectWatcher::WillDestroyCurrentMessageLoop() {
  
  
  StopWatching();
}

}  
