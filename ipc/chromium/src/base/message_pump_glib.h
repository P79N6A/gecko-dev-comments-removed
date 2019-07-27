



#ifndef BASE_MESSAGE_PUMP_GLIB_H_
#define BASE_MESSAGE_PUMP_GLIB_H_

#include "base/message_pump.h"
#include "base/observer_list.h"
#include "base/scoped_ptr.h"
#include "base/time.h"

typedef union _GdkEvent GdkEvent;
typedef struct _GMainContext GMainContext;
typedef struct _GPollFD GPollFD;
typedef struct _GSource GSource;

namespace base {



class MessagePumpForUI : public MessagePump {
 public:
  
  
  class Observer {
   public:
    virtual ~Observer() {}

    
    virtual void WillProcessEvent(GdkEvent* event) = 0;

    
    virtual void DidProcessEvent(GdkEvent* event) = 0;
  };

  
  
  
  
  
  
  
  
  class Dispatcher {
   public:
    virtual ~Dispatcher() {}
    
    
    virtual bool Dispatch(GdkEvent* event) = 0;
  };

  MessagePumpForUI();
  virtual ~MessagePumpForUI();

  
  virtual void RunWithDispatcher(Delegate* delegate, Dispatcher* dispatcher);

  virtual void Run(Delegate* delegate) { RunWithDispatcher(delegate, NULL); }
  virtual void Quit();
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

  
  
  
  
  
  
  int HandlePrepare();
  bool HandleCheck();
  void HandleDispatch();

  
  void AddObserver(Observer* observer);

  
  
  void RemoveObserver(Observer* observer);

 private:
  
  
  struct RunState {
    Delegate* delegate;
    Dispatcher* dispatcher;

    
    bool should_quit;

    
    int run_depth;

    
    
    
    bool has_work;
  };

  
  
  void WillProcessEvent(GdkEvent* event);

  
  
  void DidProcessEvent(GdkEvent* event);

  
  static void EventDispatcher(GdkEvent* event, void* data);

  RunState* state_;

  
  
  
  GMainContext* context_;

  
  TimeTicks delayed_work_time_;

  
  
  GSource* work_source_;

  
  
  
  
  int wakeup_pipe_read_;
  int wakeup_pipe_write_;
  
  scoped_ptr<GPollFD> wakeup_gpollfd_;

  
  ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpForUI);
};

}  

#endif  
