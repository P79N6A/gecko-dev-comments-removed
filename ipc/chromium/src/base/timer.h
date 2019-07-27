







































#ifndef BASE_TIMER_H_
#define BASE_TIMER_H_






#include "base/logging.h"
#include "base/task.h"
#include "base/time.h"

class MessageLoop;

namespace base {







class BaseTimer_Helper {
 public:
  
  ~BaseTimer_Helper() {
    OrphanDelayedTask();
  }

  
  bool IsRunning() const {
    return delayed_task_ != NULL;
  }

  
  
  TimeDelta GetCurrentDelay() const {
    DCHECK(IsRunning());
    return delayed_task_->delay_;
  }

 protected:
  BaseTimer_Helper() : delayed_task_(NULL) {}

  
  class TimerTask : public Task {
   public:
    TimerTask(TimeDelta delay) : delay_(delay) {
      
    }
    virtual ~TimerTask() {}
    BaseTimer_Helper* timer_;
    TimeDelta delay_;
  };

  
  void OrphanDelayedTask();

  
  
  void InitiateDelayedTask(TimerTask* timer_task);

  TimerTask* delayed_task_;

  DISALLOW_COPY_AND_ASSIGN(BaseTimer_Helper);
};




template <class Receiver, bool kIsRepeating>
class BaseTimer : public BaseTimer_Helper {
 public:
  typedef void (Receiver::*ReceiverMethod)();

  
  
  void Start(TimeDelta delay, Receiver* receiver, ReceiverMethod method) {
    DCHECK(!IsRunning());
    InitiateDelayedTask(new TimerTask(delay, receiver, method));
  }

  
  
  void Stop() {
    OrphanDelayedTask();
  }

  
  void Reset() {
    DCHECK(IsRunning());
    InitiateDelayedTask(static_cast<TimerTask*>(delayed_task_)->Clone());
  }

 private:
  typedef BaseTimer<Receiver, kIsRepeating> SelfType;

  class TimerTask : public BaseTimer_Helper::TimerTask {
   public:
    TimerTask(TimeDelta delay, Receiver* receiver, ReceiverMethod method)
        : BaseTimer_Helper::TimerTask(delay),
          receiver_(receiver),
          method_(method) {
    }

    virtual ~TimerTask() {
      
      
      
      ClearBaseTimer();
    }

    virtual void Run() {
      if (!timer_)  
        return;
      if (kIsRepeating)
        ResetBaseTimer();
      else
        ClearBaseTimer();
      DispatchToMethod(receiver_, method_, Tuple0());
    }

    TimerTask* Clone() const {
      return new TimerTask(delay_, receiver_, method_);
    }

   private:
    
    void ClearBaseTimer() {
      if (timer_) {
        SelfType* self = static_cast<SelfType*>(timer_);
        
        
        
        if (self->delayed_task_ == this)
          self->delayed_task_ = NULL;
        
        
        
        timer_ = NULL;
      }
    }

    
    void ResetBaseTimer() {
      DCHECK(timer_);
      DCHECK(kIsRepeating);
      SelfType* self = static_cast<SelfType*>(timer_);
      self->Reset();
    }

    Receiver* receiver_;
    ReceiverMethod method_;
  };
};



template <class Receiver>
class OneShotTimer : public BaseTimer<Receiver, false> {};



template <class Receiver>
class RepeatingTimer : public BaseTimer<Receiver, true> {};












template <class Receiver>
class DelayTimer {
 public:
  typedef void (Receiver::*ReceiverMethod)();

  DelayTimer(TimeDelta delay, Receiver* receiver, ReceiverMethod method)
      : receiver_(receiver),
        method_(method),
        delay_(delay) {
  }

  void Reset() {
    DelayFor(delay_);
  }

 private:
  void DelayFor(TimeDelta delay) {
    trigger_time_ = Time::Now() + delay;

    
    
    if (timer_.IsRunning() && timer_.GetCurrentDelay() <= delay)
      return;

    
    timer_.Stop();
    timer_.Start(delay, this, &DelayTimer<Receiver>::Check);
  }

  void Check() {
    if (trigger_time_.is_null())
      return;

    
    const Time now = Time::Now();
    if (now < trigger_time_) {
      DelayFor(trigger_time_ - now);
      return;
    }

    (receiver_->*method_)();
  }

  Receiver *const receiver_;
  const ReceiverMethod method_;
  const TimeDelta delay_;

  OneShotTimer<DelayTimer<Receiver> > timer_;
  Time trigger_time_;
};

}  

#endif  
