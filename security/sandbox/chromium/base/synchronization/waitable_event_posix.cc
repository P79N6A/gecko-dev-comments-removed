



#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_restrictions.h"






















namespace base {




WaitableEvent::WaitableEvent(bool manual_reset, bool initially_signaled)
    : kernel_(new WaitableEventKernel(manual_reset, initially_signaled)) {
}

WaitableEvent::~WaitableEvent() {
}

void WaitableEvent::Reset() {
  base::AutoLock locked(kernel_->lock_);
  kernel_->signaled_ = false;
}

void WaitableEvent::Signal() {
  base::AutoLock locked(kernel_->lock_);

  if (kernel_->signaled_)
    return;

  if (kernel_->manual_reset_) {
    SignalAll();
    kernel_->signaled_ = true;
  } else {
    
    
    if (!SignalOne())
      kernel_->signaled_ = true;
  }
}

bool WaitableEvent::IsSignaled() {
  base::AutoLock locked(kernel_->lock_);

  const bool result = kernel_->signaled_;
  if (result && !kernel_->manual_reset_)
    kernel_->signaled_ = false;
  return result;
}








class SyncWaiter : public WaitableEvent::Waiter {
 public:
  SyncWaiter()
      : fired_(false),
        signaling_event_(NULL),
        lock_(),
        cv_(&lock_) {
  }

  bool Fire(WaitableEvent* signaling_event) override {
    base::AutoLock locked(lock_);

    if (fired_)
      return false;

    fired_ = true;
    signaling_event_ = signaling_event;

    cv_.Broadcast();

    
    
    

    return true;
  }

  WaitableEvent* signaling_event() const {
    return signaling_event_;
  }

  
  
  
  
  bool Compare(void* tag) override { return this == tag; }

  
  
  
  bool fired() const {
    return fired_;
  }

  
  
  
  
  
  void Disable() {
    fired_ = true;
  }

  base::Lock* lock() {
    return &lock_;
  }

  base::ConditionVariable* cv() {
    return &cv_;
  }

 private:
  bool fired_;
  WaitableEvent* signaling_event_;  
  base::Lock lock_;
  base::ConditionVariable cv_;
};

void WaitableEvent::Wait() {
  bool result = TimedWait(TimeDelta::FromSeconds(-1));
  DCHECK(result) << "TimedWait() should never fail with infinite timeout";
}

bool WaitableEvent::TimedWait(const TimeDelta& max_time) {
  base::ThreadRestrictions::AssertWaitAllowed();
  const TimeTicks end_time(TimeTicks::Now() + max_time);
  const bool finite_time = max_time.ToInternalValue() >= 0;

  kernel_->lock_.Acquire();
  if (kernel_->signaled_) {
    if (!kernel_->manual_reset_) {
      
      
      kernel_->signaled_ = false;
    }

    kernel_->lock_.Release();
    return true;
  }

  SyncWaiter sw;
  sw.lock()->Acquire();

  Enqueue(&sw);
  kernel_->lock_.Release();
  
  
  

  for (;;) {
    const TimeTicks current_time(TimeTicks::Now());

    if (sw.fired() || (finite_time && current_time >= end_time)) {
      const bool return_value = sw.fired();

      
      
      
      
      
      sw.Disable();
      sw.lock()->Release();

      
      
      
      
      
      kernel_->lock_.Acquire();
      kernel_->Dequeue(&sw, &sw);
      kernel_->lock_.Release();

      return return_value;
    }

    if (finite_time) {
      const TimeDelta max_wait(end_time - current_time);
      sw.cv()->TimedWait(max_wait);
    } else {
      sw.cv()->Wait();
    }
  }
}




static bool  
cmp_fst_addr(const std::pair<WaitableEvent*, unsigned> &a,
             const std::pair<WaitableEvent*, unsigned> &b) {
  return a.first < b.first;
}


size_t WaitableEvent::WaitMany(WaitableEvent** raw_waitables,
                               size_t count) {
  base::ThreadRestrictions::AssertWaitAllowed();
  DCHECK(count) << "Cannot wait on no events";

  
  
  
  std::vector<std::pair<WaitableEvent*, size_t> > waitables;
  waitables.reserve(count);
  for (size_t i = 0; i < count; ++i)
    waitables.push_back(std::make_pair(raw_waitables[i], i));

  DCHECK_EQ(count, waitables.size());

  sort(waitables.begin(), waitables.end(), cmp_fst_addr);

  
  
  
  for (size_t i = 0; i < waitables.size() - 1; ++i) {
    DCHECK(waitables[i].first != waitables[i+1].first);
  }

  SyncWaiter sw;

  const size_t r = EnqueueMany(&waitables[0], count, &sw);
  if (r) {
    
    
    
    
    return waitables[count - r].second;
  }

  
  
  sw.lock()->Acquire();
    
    for (size_t i = 0; i < count; ++i) {
      waitables[count - (1 + i)].first->kernel_->lock_.Release();
    }

    for (;;) {
      if (sw.fired())
        break;

      sw.cv()->Wait();
    }
  sw.lock()->Release();

  
  WaitableEvent *const signaled_event = sw.signaling_event();
  
  size_t signaled_index = 0;

  
  
  for (size_t i = 0; i < count; ++i) {
    if (raw_waitables[i] != signaled_event) {
      raw_waitables[i]->kernel_->lock_.Acquire();
        
        
        
        raw_waitables[i]->kernel_->Dequeue(&sw, &sw);
      raw_waitables[i]->kernel_->lock_.Release();
    } else {
      
      
      
      raw_waitables[i]->kernel_->lock_.Acquire();
      raw_waitables[i]->kernel_->lock_.Release();
      signaled_index = i;
    }
  }

  return signaled_index;
}












size_t WaitableEvent::EnqueueMany
    (std::pair<WaitableEvent*, size_t>* waitables,
     size_t count, Waiter* waiter) {
  if (!count)
    return 0;

  waitables[0].first->kernel_->lock_.Acquire();
    if (waitables[0].first->kernel_->signaled_) {
      if (!waitables[0].first->kernel_->manual_reset_)
        waitables[0].first->kernel_->signaled_ = false;
      waitables[0].first->kernel_->lock_.Release();
      return count;
    }

    const size_t r = EnqueueMany(waitables + 1, count - 1, waiter);
    if (r) {
      waitables[0].first->kernel_->lock_.Release();
    } else {
      waitables[0].first->Enqueue(waiter);
    }

    return r;
}







WaitableEvent::WaitableEventKernel::WaitableEventKernel(bool manual_reset,
                                                        bool initially_signaled)
    : manual_reset_(manual_reset),
      signaled_(initially_signaled) {
}

WaitableEvent::WaitableEventKernel::~WaitableEventKernel() {
}




bool WaitableEvent::SignalAll() {
  bool signaled_at_least_one = false;

  for (std::list<Waiter*>::iterator
       i = kernel_->waiters_.begin(); i != kernel_->waiters_.end(); ++i) {
    if ((*i)->Fire(this))
      signaled_at_least_one = true;
  }

  kernel_->waiters_.clear();
  return signaled_at_least_one;
}





bool WaitableEvent::SignalOne() {
  for (;;) {
    if (kernel_->waiters_.empty())
      return false;

    const bool r = (*kernel_->waiters_.begin())->Fire(this);
    kernel_->waiters_.pop_front();
    if (r)
      return true;
  }
}




void WaitableEvent::Enqueue(Waiter* waiter) {
  kernel_->waiters_.push_back(waiter);
}





bool WaitableEvent::WaitableEventKernel::Dequeue(Waiter* waiter, void* tag) {
  for (std::list<Waiter*>::iterator
       i = waiters_.begin(); i != waiters_.end(); ++i) {
    if (*i == waiter && (*i)->Compare(tag)) {
      waiters_.erase(i);
      return true;
    }
  }

  return false;
}



}  
