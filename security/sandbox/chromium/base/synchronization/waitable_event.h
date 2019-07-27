



#ifndef BASE_SYNCHRONIZATION_WAITABLE_EVENT_H_
#define BASE_SYNCHRONIZATION_WAITABLE_EVENT_H_

#include "base/base_export.h"
#include "base/basictypes.h"

#if defined(OS_WIN)
#include "base/win/scoped_handle.h"
#endif

#if defined(OS_POSIX)
#include <list>
#include <utility>
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#endif

namespace base {


static const int kNoTimeout = -1;

class TimeDelta;
















class BASE_EXPORT WaitableEvent {
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

  
  
  
  
  
  
  
  
  void Wait();

  
  
  
  
  
  bool TimedWait(const TimeDelta& max_time);

#if defined(OS_WIN)
  HANDLE handle() const { return handle_.Get(); }
#endif

  
  
  
  
  
  
  
  
  
  static size_t WaitMany(WaitableEvent** waitables, size_t count);

  

  
  
  
  class Waiter {
   public:
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual bool Fire(WaitableEvent* signaling_event) = 0;

    
    
    
    
    virtual bool Compare(void* tag) = 0;

   protected:
    virtual ~Waiter() {}
  };

 private:
  friend class WaitableEventWatcher;

#if defined(OS_WIN)
  win::ScopedHandle handle_;
#else
  
  
  
  
  
  
  
  
  struct WaitableEventKernel :
      public RefCountedThreadSafe<WaitableEventKernel> {
   public:
    WaitableEventKernel(bool manual_reset, bool initially_signaled);

    bool Dequeue(Waiter* waiter, void* tag);

    base::Lock lock_;
    const bool manual_reset_;
    bool signaled_;
    std::list<Waiter*> waiters_;

   private:
    friend class RefCountedThreadSafe<WaitableEventKernel>;
    ~WaitableEventKernel();
  };

  typedef std::pair<WaitableEvent*, size_t> WaiterAndIndex;

  
  
  
  
  
  static size_t EnqueueMany(WaiterAndIndex* waitables,
                            size_t count, Waiter* waiter);

  bool SignalAll();
  bool SignalOne();
  void Enqueue(Waiter* waiter);

  scoped_refptr<WaitableEventKernel> kernel_;
#endif

  DISALLOW_COPY_AND_ASSIGN(WaitableEvent);
};

}  

#endif  
