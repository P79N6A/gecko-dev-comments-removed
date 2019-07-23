




























#ifndef BASE_MESSAGE_PUMP_MAC_H_
#define BASE_MESSAGE_PUMP_MAC_H_

#include "base/message_pump.h"

#include <CoreFoundation/CoreFoundation.h>

namespace base {

class Time;

class MessagePumpCFRunLoopBase : public MessagePump {
 public:
  MessagePumpCFRunLoopBase();
  virtual ~MessagePumpCFRunLoopBase();

  
  
  
  
  virtual void Run(Delegate* delegate);
  virtual void DoRun(Delegate* delegate) = 0;

  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const Time& delayed_work_time);

 protected:
  
  CFRunLoopRef run_loop_;

 private:
  
  
  
  static void RunDelayedWorkTimer(CFRunLoopTimerRef timer, void* info);

  
  
  static void RunWork(void* info);

  
  
  
  static void RunDelayedWork(void* info);

  
  
  static void RunIdleWork(CFRunLoopObserverRef observer,
                          CFRunLoopActivity activity, void* info);

  
  
  CFRunLoopTimerRef delayed_work_timer_;
  CFRunLoopSourceRef work_source_;
  CFRunLoopSourceRef delayed_work_source_;
  CFRunLoopObserverRef idle_work_observer_;

  
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpCFRunLoopBase);
};

class MessagePumpCFRunLoop : public MessagePumpCFRunLoopBase {
 public:
  MessagePumpCFRunLoop();
  virtual ~MessagePumpCFRunLoop();

  virtual void DoRun(Delegate* delegate);
  virtual void Quit();

 private:
  
  
  
  
  static void EnterExitRunLoop(CFRunLoopObserverRef observer,
                               CFRunLoopActivity activity, void* info);

  
  CFRunLoopObserverRef enter_exit_observer_;

  
  
  
  int nesting_level_;

  
  
  int innermost_quittable_;

  
  
  
  bool quit_pending_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpCFRunLoop);
};

class MessagePumpNSRunLoop : public MessagePumpCFRunLoopBase {
 public:
  MessagePumpNSRunLoop();
  virtual ~MessagePumpNSRunLoop();

  virtual void DoRun(Delegate* delegate);
  virtual void Quit();

 private:
  
  
  
  CFRunLoopSourceRef quit_source_;

  
  bool keep_running_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpNSRunLoop);
};

class MessagePumpNSApplication : public MessagePumpCFRunLoopBase {
 public:
  MessagePumpNSApplication();

  virtual void DoRun(Delegate* delegate);
  virtual void Quit();

 private:
  
  bool keep_running_;

  
  
  
  
  bool running_own_loop_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpNSApplication);
};

class MessagePumpMac {
 public:
  
  
  static MessagePump* Create();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MessagePumpMac);
};

}  

#endif  
