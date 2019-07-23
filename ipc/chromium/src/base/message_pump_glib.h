



#ifndef BASE_MESSAGE_PUMP_GLIB_H_
#define BASE_MESSAGE_PUMP_GLIB_H_

#include <glib.h>

#include "base/message_pump.h"
#include "base/time.h"

namespace base {



class MessagePumpForUI : public MessagePump {
 public:
  MessagePumpForUI();
  ~MessagePumpForUI();

  virtual void Run(Delegate* delegate);
  virtual void Quit();
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const Time& delayed_work_time);

  
  
  
  
  
  int HandlePrepare();
  void HandleDispatch();

 private:
  
  
  struct RunState {
    Delegate* delegate;

    
    bool should_quit;

    
    int run_depth;

    
    
    bool more_work_is_plausible;
  };

  RunState* state_;

  
  
  
  GMainContext* context_;

  
  Time delayed_work_time_;

  
  
  GSource* work_source_;

  
  
  
  
  int wakeup_pipe_read_;
  int wakeup_pipe_write_;
  GPollFD wakeup_gpollfd_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpForUI);
};

}  

#endif  
