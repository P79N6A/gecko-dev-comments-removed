



#ifndef BASE_MESSAGE_PUMP_H_
#define BASE_MESSAGE_PUMP_H_

#include "base/ref_counted.h"

namespace base {

class Time;

class MessagePump : public RefCountedThreadSafe<MessagePump> {
 public:
  
  
  class Delegate {
   public:
    virtual ~Delegate() {}

    
    
    
    virtual bool DoWork() = 0;

    
    
    
    
    
    
    
    
    virtual bool DoDelayedWork(Time* next_delayed_work_time) = 0;

    
    
    virtual bool DoIdleWork() = 0;
  };

  virtual ~MessagePump() {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void Run(Delegate* delegate) = 0;

  
  
  virtual void Quit() = 0;

  
  
  
  
  virtual void ScheduleWork() = 0;

  
  
  
  virtual void ScheduleDelayedWork(const Time& delayed_work_time) = 0;
};

}  

#endif  
