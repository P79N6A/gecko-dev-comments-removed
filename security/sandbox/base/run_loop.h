



#ifndef BASE_RUN_LOOP_H_
#define BASE_RUN_LOOP_H_

#include "base/base_export.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"

namespace base {
#if defined(OS_ANDROID)
class MessagePumpForUI;
#endif

#if defined(OS_IOS)
class MessagePumpUIApplication;
#endif






class BASE_EXPORT RunLoop {
 public:
  RunLoop();
#if !defined(OS_MACOSX) && !defined(OS_ANDROID) && \
    !defined(USE_GTK_MESSAGE_PUMP)
  explicit RunLoop(MessageLoop::Dispatcher* dispatcher);
#endif
  ~RunLoop();

#if !defined(OS_MACOSX) && !defined(OS_ANDROID) && \
    !defined(USE_GTK_MESSAGE_PUMP)
  void set_dispatcher(MessageLoop::Dispatcher* dispatcher) {
    dispatcher_ = dispatcher;
  }
#endif

  
  
  
  
  void Run();

  
  
  
  void RunUntilIdle();

  bool running() const { return running_; }

  
  
  
  
  
  
  
  
  
  
  void Quit();

  
  
  
  
  
  
  
  base::Closure QuitClosure();

 private:
  friend class MessageLoop;
#if defined(OS_ANDROID)
  
  
  friend class base::MessagePumpForUI;
#endif

#if defined(OS_IOS)
  
  
  friend class base::MessagePumpUIApplication;
#endif

  
  bool BeforeRun();
  void AfterRun();

  MessageLoop* loop_;

  
  base::WeakPtrFactory<RunLoop> weak_factory_;

  
  RunLoop* previous_run_loop_;

#if !defined(OS_MACOSX) && !defined(OS_ANDROID) && \
    !defined(USE_GTK_MESSAGE_PUMP)
  MessageLoop::Dispatcher* dispatcher_;
#endif

  
  int run_depth_;

  bool run_called_;
  bool quit_called_;
  bool running_;

  
  
  bool quit_when_idle_received_;

  DISALLOW_COPY_AND_ASSIGN(RunLoop);
};

}  

#endif  
