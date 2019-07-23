



#ifndef CHROME_COMMON_CHILD_PROCESS_H__
#define CHROME_COMMON_CHILD_PROCESS_H__

#include <string>
#include <vector>
#include "base/basictypes.h"
#include "base/message_loop.h"
#include "base/scoped_ptr.h"
#include "base/waitable_event.h"

class ChildThread;




class ChildProcess {
 public:
  
  
  ChildProcess(ChildThread* child_thread);
  virtual ~ChildProcess();

  
  ChildThread* child_thread() { return child_thread_.get(); }

  
  
  
  
  
  
  
  
  
  base::WaitableEvent* GetShutDownEvent();

  
  
  
  
  void AddRefProcess();
  void ReleaseProcess();

  
  static ChildProcess* current() { return child_process_; }

 private:
  
  
  scoped_ptr<ChildThread> child_thread_;

  int ref_count_;

  
  base::WaitableEvent shutdown_event_;

  
  static ChildProcess* child_process_;

  DISALLOW_EVIL_CONSTRUCTORS(ChildProcess);
};

#endif  
