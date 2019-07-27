



#ifndef BASE_WAITABLE_EVENT_H_
#define BASE_WAITABLE_EVENT_H_

#include "base/basictypes.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(OS_POSIX)
#include <list>
#include <utility>
#include "base/condition_variable.h"
#include "base/lock.h"
#include "base/ref_counted.h"
#endif

#include "base/message_loop.h"

namespace base {


static const int kNoTimeout = -1;

class TimeDelta;
















class WaitableEvent {
 public:
  
  
  
  
  WaitableEvent(bool manual_reset, bool initially_signaled);

#if defined(OS_WIN)
  
  
  
  explicit WaitableEvent(HANDLE event_handle);

  
  HANDLE Release();
#endif

  ~WaitableEvent();

  
  void Reset();

  
  
  void Signal();

  
  
  bool IsSignaled();

  
  
  bool Wait();

  
  
  
  bool TimedWait(const TimeDelta& max_time);

#if defined(OS_WIN)
  HANDLE handle() const { return handle_; }
#endif

  
  
  
  
  
  
  
  
  static size_t WaitMany(WaitableEvent** waitables, size_t count);

  

  
  
  
  class Waiter {
   public:
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual bool Fire(WaitableEvent* signaling_event) = 0;

    
    
    
    
    virtual bool Compare(void* tag) = 0;
  };

 private:
  friend class WaitableEventWatcher;

#if defined(OS_WIN)
  HANDLE handle_;
#else
  
  
  
  
  
  
  
  
  struct WaitableEventKernel :
      public RefCountedThreadSafe<WaitableEventKernel> {
   public:
    WaitableEventKernel(bool manual_reset, bool initially_signaled)
        : manual_reset_(manual_reset),
          signaled_(initially_signaled) {
    }

    bool Dequeue(Waiter* waiter, void* tag);

    Lock lock_;
    const bool manual_reset_;
    bool signaled_;
    std::list<Waiter*> waiters_;
  };

  scoped_refptr<WaitableEventKernel> kernel_;

  bool SignalAll();
  bool SignalOne();
  void Enqueue(Waiter* waiter);

  
  
  
  
  
  typedef std::pair<WaitableEvent*, size_t> WaiterAndIndex;
  static size_t EnqueueMany(WaiterAndIndex* waitables,
                            size_t count, Waiter* waiter);
#endif

  DISALLOW_COPY_AND_ASSIGN(WaitableEvent);
};

}  

#endif  
