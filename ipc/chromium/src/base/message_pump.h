



#ifndef BASE_MESSAGE_PUMP_H_
#define BASE_MESSAGE_PUMP_H_

#include "nsISupportsImpl.h"

namespace base {

class TimeTicks;

class MessagePump {
 public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MessagePump)

  
  
  class Delegate {
   public:
    virtual ~Delegate() {}

    
    
    
    virtual bool DoWork() = 0;

    
    
    
    
    
    
    
    
    virtual bool DoDelayedWork(TimeTicks* next_delayed_work_time) = 0;

    
    
    virtual bool DoIdleWork() = 0;
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void Run(Delegate* delegate) = 0;

  
  
  virtual void Quit() = 0;

  
  
  
  
  virtual void ScheduleWork() = 0;

  
  
  
  
  
  
  virtual void ScheduleWorkForNestedLoop() { ScheduleWork(); };

  
  
  
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time) = 0;

protected:
  virtual ~MessagePump() {};
};

}  

#endif  
