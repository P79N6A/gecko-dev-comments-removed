



#include "base/waitable_event.h"

#include "base/condition_variable.h"
#include "base/lock.h"
#include "base/message_loop.h"






















namespace base {




WaitableEvent::WaitableEvent(bool manual_reset, bool initially_signaled)
    : kernel_(new WaitableEventKernel(manual_reset, initially_signaled)) {
}

WaitableEvent::~WaitableEvent() {
}

void WaitableEvent::Reset() {
  AutoLock locked(kernel_->lock_);
  kernel_->signaled_ = false;
}

void WaitableEvent::Signal() {
  AutoLock locked(kernel_->lock_);

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
  AutoLock locked(kernel_->lock_);

  const bool result = kernel_->signaled_;
  if (result && !kernel_->manual_reset_)
    kernel_->signaled_ = false;
  return result;
}








class SyncWaiter : public WaitableEvent::Waiter {
 public:
  SyncWaiter(ConditionVariable* cv, Lock* lock)
      : fired_(false),
        cv_(cv),
        lock_(lock),
        signaling_event_(NULL) {
  }

  bool Fire(WaitableEvent *signaling_event) {
    lock_->Acquire();
      const bool previous_value = fired_;
      fired_ = true;
      if (!previous_value)
        signaling_event_ = signaling_event;
    lock_->Release();

    if (previous_value)
      return false;

    cv_->Broadcast();

    
    return true;
  }

  WaitableEvent* signaled_event() const {
    return signaling_event_;
  }

  
  
  
  
  bool Compare(void* tag) {
    return this == tag;
  }

  
  
  
  bool fired() const {
    return fired_;
  }

  
  
  
  
  
  void Disable() {
    fired_ = true;
  }

 private:
  bool fired_;
  ConditionVariable *const cv_;
  Lock *const lock_;
  WaitableEvent* signaling_event_;  
};

bool WaitableEvent::TimedWait(const TimeDelta& max_time) {
  const Time end_time(Time::Now() + max_time);
  const bool finite_time = max_time.ToInternalValue() >= 0;

  kernel_->lock_.Acquire();
    if (kernel_->signaled_) {
      if (!kernel_->manual_reset_) {
        
        
        kernel_->signaled_ = false;
      }

      kernel_->lock_.Release();
      return true;
    }

    Lock lock;
    lock.Acquire();
    ConditionVariable cv(&lock);
    SyncWaiter sw(&cv, &lock);

    Enqueue(&sw);
  kernel_->lock_.Release();
  
  
  

  for (;;) {
    const Time current_time(Time::Now());

    if (sw.fired() || (finite_time && current_time >= end_time)) {
      const bool return_value = sw.fired();

      
      
      
      
      
      sw.Disable();
      lock.Release();

      kernel_->lock_.Acquire();
        kernel_->Dequeue(&sw, &sw);
      kernel_->lock_.Release();

      return return_value;
    }

    if (finite_time) {
      const TimeDelta max_wait(end_time - current_time);
      cv.TimedWait(max_wait);
    } else {
      cv.Wait();
    }
  }
}

bool WaitableEvent::Wait() {
  return TimedWait(TimeDelta::FromSeconds(-1));
}







static bool  
cmp_fst_addr(const std::pair<WaitableEvent*, unsigned> &a,
             const std::pair<WaitableEvent*, unsigned> &b) {
  return a.first < b.first;
}


size_t WaitableEvent::WaitMany(WaitableEvent** raw_waitables,
                               size_t count) {
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

  Lock lock;
  ConditionVariable cv(&lock);
  SyncWaiter sw(&cv, &lock);

  const size_t r = EnqueueMany(&waitables[0], count, &sw);
  if (r) {
    
    
    
    
    return waitables[count - r].second;
  }

  
  
  lock.Acquire();
    
    for (size_t i = 0; i < count; ++i) {
      waitables[count - (1 + i)].first->kernel_->lock_.Release();
    }

    for (;;) {
      if (sw.fired())
        break;

      cv.Wait();
    }
  lock.Release();

  
  WaitableEvent *const signaled_event = sw.signaled_event();
  
  size_t signaled_index = 0;

  
  
  for (size_t i = 0; i < count; ++i) {
    if (raw_waitables[i] != signaled_event) {
      raw_waitables[i]->kernel_->lock_.Acquire();
        
        
        
        raw_waitables[i]->kernel_->Dequeue(&sw, &sw);
      raw_waitables[i]->kernel_->lock_.Release();
    } else {
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
