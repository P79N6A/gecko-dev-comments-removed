



#ifndef BASE_MESSAGE_PUMP_ANDROID_H_
#define BASE_MESSAGE_PUMP_ANDROID_H_

#include "base/message_pump.h"
#include "base/time.h"

namespace base {

class MessagePumpForUI;

class MessagePumpAndroid {

 public:
  MessagePumpAndroid(MessagePumpForUI &pump);
  ~MessagePumpAndroid();

 private:
  base::MessagePumpForUI &pump;
};



class MessagePumpForUI : public MessagePump {

 public:
  MessagePumpForUI();
  ~MessagePumpForUI();

  virtual void Run(Delegate* delegate);
  virtual void Quit();
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

  
  
  
  void HandleDispatch();

 private:
  
  
  struct RunState {
    Delegate* delegate;

    
    bool should_quit;

    
    int run_depth;

    
    
    bool more_work_is_plausible;
  };

  RunState* state_;

  
  TimeTicks delayed_work_time_;

  bool work_scheduled;

  
  MessagePumpAndroid pump;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpForUI);
};

}  

#endif  
