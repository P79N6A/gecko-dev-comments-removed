



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
  virtual void ScheduleDelayedWork(const Time& delayed_work_time);

 private:
  
  bool keep_running_;

  
  WaitableEvent event_;

  
  Time delayed_work_time_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpDefault);
};

}  

#endif  
