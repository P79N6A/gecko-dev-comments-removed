



#ifndef BASE_WAITABLE_EVENT_WATCHER_H_
#define BASE_WAITABLE_EVENT_WATCHER_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/object_watcher.h"
#else
#include "base/message_loop.h"
#include "base/waitable_event.h"
#endif

namespace base {

class Flag;
class AsyncWaiter;
class AsyncCallbackTask;
class WaitableEvent;





































class WaitableEventWatcher
#if defined(OS_POSIX)
    : public MessageLoop::DestructionObserver
#endif
{
 public:

  WaitableEventWatcher();
  ~WaitableEventWatcher();

  class Delegate {
   public:
    virtual ~Delegate() { }

    
    
    
    
    
    
    
    
    virtual void OnWaitableEventSignaled(WaitableEvent* waitable_event) = 0;
  };

  
  
  
  
  
  bool StartWatching(WaitableEvent* event, Delegate* delegate);

  
  
  
  
  
  
  
  
  
  void StopWatching();

  
  
  
  
  WaitableEvent* GetWatchedEvent();

 private:
  WaitableEvent* event_;

#if defined(OS_WIN)
  
  
  
  
  
  
  class ObjectWatcherHelper : public ObjectWatcher::Delegate {
   public:
    ObjectWatcherHelper(WaitableEventWatcher* watcher);

    
    
    
    void OnObjectSignaled(HANDLE h);

   private:
    WaitableEventWatcher *const watcher_;
  };

  void OnObjectSignaled();

  Delegate* delegate_;
  ObjectWatcherHelper helper_;
  ObjectWatcher watcher_;
#else
  
  
  
  void WillDestroyCurrentMessageLoop();

  MessageLoop* message_loop_;
  scoped_refptr<Flag> cancel_flag_;
  AsyncWaiter* waiter_;
  AsyncCallbackTask* callback_task_;
  scoped_refptr<WaitableEvent::WaitableEventKernel> kernel_;
#endif
};

}  

#endif  
