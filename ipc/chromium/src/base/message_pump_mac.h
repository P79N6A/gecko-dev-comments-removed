




























#ifndef BASE_MESSAGE_PUMP_MAC_H_
#define BASE_MESSAGE_PUMP_MAC_H_

#include "base/message_pump.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#if defined(__OBJC__)
@class NSAutoreleasePool;
#else  
class NSAutoreleasePool;
#endif  

namespace base {

class TimeTicks;

class MessagePumpCFRunLoopBase : public MessagePump {
  
  friend class MessagePumpScopedAutoreleasePool;
 public:
  MessagePumpCFRunLoopBase();
  virtual ~MessagePumpCFRunLoopBase();

  
  
  
  
  virtual void Run(Delegate* delegate);
  virtual void DoRun(Delegate* delegate) = 0;

  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

 protected:
  
  CFRunLoopRef run_loop() const { return run_loop_; }
  int nesting_level() const { return nesting_level_; }
  int run_nesting_level() const { return run_nesting_level_; }

  
  
  
  
  virtual NSAutoreleasePool* CreateAutoreleasePool();

 private:
  
  
  
  static void RunDelayedWorkTimer(CFRunLoopTimerRef timer, void* info);

  
  
  
  static void RunWorkSource(void* info);
  bool RunWork();

  
  
  
  
  
  static void RunDelayedWorkSource(void* info);
  bool RunDelayedWork();

  
  
  
  
  
  static void RunIdleWorkSource(void* info);
  bool RunIdleWork();

  
  
  
  
  
  
  static void RunNestingDeferredWorkSource(void* info);
  bool RunNestingDeferredWork();

  
  
  
  
  
  void MaybeScheduleNestingDeferredWork();

  
  
  static void PreWaitObserver(CFRunLoopObserverRef observer,
                              CFRunLoopActivity activity, void* info);

  
  
  static void PreSourceObserver(CFRunLoopObserverRef observer,
                                CFRunLoopActivity activity, void* info);

  
  
  
  static void EnterExitObserver(CFRunLoopObserverRef observer,
                                CFRunLoopActivity activity, void* info);

  
  
  
  virtual void EnterExitRunLoop(CFRunLoopActivity activity);

  
  
  static void PowerStateNotification(void* info, io_service_t service,
                                     uint32_t message_type,
                                     void* message_argument);

  
  CFRunLoopRef run_loop_;

  
  
  CFRunLoopTimerRef delayed_work_timer_;
  CFRunLoopSourceRef work_source_;
  CFRunLoopSourceRef delayed_work_source_;
  CFRunLoopSourceRef idle_work_source_;
  CFRunLoopSourceRef nesting_deferred_work_source_;
  CFRunLoopObserverRef pre_wait_observer_;
  CFRunLoopObserverRef pre_source_observer_;
  CFRunLoopObserverRef enter_exit_observer_;

  
  io_connect_t root_power_domain_;
  IONotificationPortRef power_notification_port_;
  io_object_t power_notification_object_;

  
  Delegate* delegate_;

  
  
  
  
  CFAbsoluteTime delayed_work_fire_time_;

  
  
  
  int nesting_level_;

  
  
  int run_nesting_level_;

  
  
  int deepest_nesting_level_;

  
  
  
  
  
  bool delegateless_work_;
  bool delegateless_delayed_work_;
  bool delegateless_idle_work_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpCFRunLoopBase);
};

class MessagePumpCFRunLoop : public MessagePumpCFRunLoopBase {
 public:
  MessagePumpCFRunLoop();

  virtual void DoRun(Delegate* delegate);
  virtual void Quit();

 private:
  virtual void EnterExitRunLoop(CFRunLoopActivity activity);

  
  
  
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

 protected:
  
  virtual NSAutoreleasePool* CreateAutoreleasePool();

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
