



#ifndef BASE_MESSAGE_PUMP_WIN_H_
#define BASE_MESSAGE_PUMP_WIN_H_

#include <windows.h>

#include <list>

#include "base/lock.h"
#include "base/message_pump.h"
#include "base/observer_list.h"
#include "base/scoped_handle.h"
#include "base/time.h"

namespace base {




class MessagePumpWin : public MessagePump {
 public:
  
  
  
  
  
  class Observer {
   public:
    virtual ~Observer() {}

    
    
    virtual void WillProcessMessage(const MSG& msg) = 0;

    
    
    virtual void DidProcessMessage(const MSG& msg) = 0;
  };

  
  
  
  
  
  
  
  
  class Dispatcher {
   public:
    virtual ~Dispatcher() {}
    
    
    virtual bool Dispatch(const MSG& msg) = 0;
  };

  MessagePumpWin() : have_work_(0), state_(NULL) {}
  virtual ~MessagePumpWin() {}

  
  void AddObserver(Observer* observer);

  
  
  void RemoveObserver(Observer* observer);

  
  
  void WillProcessMessage(const MSG& msg);
  void DidProcessMessage(const MSG& msg);

  
  void RunWithDispatcher(Delegate* delegate, Dispatcher* dispatcher);

  
  virtual void Run(Delegate* delegate) { RunWithDispatcher(delegate, NULL); }
  virtual void Quit();

 protected:
  struct RunState {
    Delegate* delegate;
    Dispatcher* dispatcher;

    
    bool should_quit;

    
    int run_depth;
  };

  virtual void DoRunLoop() = 0;
  int GetCurrentDelay() const;

  ObserverList<Observer> observers_;

  
  Time delayed_work_time_;

  
  
  
  LONG have_work_;

  
  RunState* state_;
};

















































class MessagePumpForUI : public MessagePumpWin {
 public:
  MessagePumpForUI();
  virtual ~MessagePumpForUI();

  
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const Time& delayed_work_time);

  
  
  
  void PumpOutPendingPaintMessages();

 private:
  static LRESULT CALLBACK WndProcThunk(
      HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
  virtual void DoRunLoop();
  void InitMessageWnd();
  void WaitForWork();
  void HandleWorkMessage();
  void HandleTimerMessage();
  bool ProcessNextWindowsMessage();
  bool ProcessMessageHelper(const MSG& msg);
  bool ProcessPumpReplacementMessage();

  
  HWND message_hwnd_;
};







class MessagePumpForIO : public MessagePumpWin {
 public:
  struct IOContext;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  class IOHandler {
   public:
    virtual ~IOHandler() {}
    
    
    
    
    virtual void OnIOCompleted(IOContext* context, DWORD bytes_transfered,
                               DWORD error) = 0;
  };

  
  
  
  
  
  
  
  
  
  struct IOContext {
    OVERLAPPED overlapped;
    IOHandler* handler;
  };

  MessagePumpForIO();
  virtual ~MessagePumpForIO() {}

  
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const Time& delayed_work_time);

  
  
  
  void RegisterIOHandler(HANDLE file_handle, IOHandler* handler);

  
  
  
  
  
  
  
  
  
  bool WaitForIOCompletion(DWORD timeout, IOHandler* filter);

 private:
  struct IOItem {
    IOHandler* handler;
    IOContext* context;
    DWORD bytes_transfered;
    DWORD error;
  };

  virtual void DoRunLoop();
  void WaitForWork();
  bool MatchCompletedIOItem(IOHandler* filter, IOItem* item);
  bool GetIOItem(DWORD timeout, IOItem* item);
  bool ProcessInternalIOItem(const IOItem& item);

  
  ScopedHandle port_;
  
  
  std::list<IOItem> completed_io_;
};

}  

#endif  
