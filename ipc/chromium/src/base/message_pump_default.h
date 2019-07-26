



#ifndef BASE_MESSAGE_PUMP_DEFAULT_H_
#define BASE_MESSAGE_PUMP_DEFAULT_H_

#include "base/message_pump.h"
#include "base/time.h"
#include "base/waitable_event.h"

namespace base {

class MessagePumpDefault : public MessagePump {
 public:
  MessagePumpDefault();
  ~MessagePumpDefault() {}

  
  virtual void Run(Delegate* delegate);
  virtual void Quit();
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

 protected:
  
  bool keep_running_;

  
  WaitableEvent event_;

  
  TimeTicks delayed_work_time_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MessagePumpDefault);
};

}  

#endif  
