



#ifndef BASE_MESSAGE_PUMP_H_
#define BASE_MESSAGE_PUMP_H_

#include "base/ref_counted.h"

namespace base {

class TimeTicks;

class MessagePump : public RefCountedThreadSafe<MessagePump> {
 public:
  
  
  class Delegate {
   public:
    virtual ~Delegate() {}

    
    
    
    virtual bool DoWork() = 0;

    
    
    
    
    
    
    
    
    virtual bool DoDelayedWork(TimeTicks* next_delayed_work_time) = 0;

    
    
    virtual bool DoIdleWork() = 0;
  };

  virtual ~MessagePump() {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void Run(Delegate* delegate) = 0;

  
  
  virtual void Quit() = 0;

  
  
  
  
  virtual void ScheduleWork() = 0;

  
  
  
  
  
  
  virtual void ScheduleWorkForNestedLoop() { ScheduleWork(); };

  
  
  
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time) = 0;
};

}  

#endif  
