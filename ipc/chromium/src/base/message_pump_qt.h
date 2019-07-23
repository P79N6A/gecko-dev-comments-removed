



#ifndef BASE_MESSAGE_PUMP_QT_H_
#define BASE_MESSAGE_PUMP_QT_H_

#include <qobject.h>

#include "base/message_pump.h"
#include "base/time.h"

namespace base {

class MessagePumpForUI;

class MessagePumpQt : public QObject {
  Q_OBJECT

 public:
  MessagePumpQt(MessagePumpForUI &pump);
  ~MessagePumpQt();

  virtual bool event (QEvent *e);

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
  virtual void ScheduleDelayedWork(const Time& delayed_work_time);

  
  
  
  void HandleDispatch();

 private:
  
  
  struct RunState {
    Delegate* delegate;

    
    bool should_quit;

    
    int run_depth;

    
    
    bool more_work_is_plausible;
  };

  RunState* state_;

  
  Time delayed_work_time_;

  
  
  
  MessagePumpQt qt_pump;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpForUI);
};

}  

#endif  
