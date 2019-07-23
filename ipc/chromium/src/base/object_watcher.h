



#ifndef BASE_OBJECT_WATCHER_H_
#define BASE_OBJECT_WATCHER_H_

#include <windows.h>

#include "base/message_loop.h"

namespace base {





























class ObjectWatcher : public MessageLoop::DestructionObserver {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}
    
    
    virtual void OnObjectSignaled(HANDLE object) = 0;
  };

  ObjectWatcher();
  ~ObjectWatcher();

  
  
  
  
  
  
  bool StartWatching(HANDLE object, Delegate* delegate);

  
  
  
  
  
  
  bool StopWatching();

  
  
  HANDLE GetWatchedObject();

 private:
  
  static void CALLBACK DoneWaiting(void* param, BOOLEAN timed_out);

  
  virtual void WillDestroyCurrentMessageLoop();

  
  struct Watch;
  Watch* watch_;

  DISALLOW_COPY_AND_ASSIGN(ObjectWatcher);
};

}  

#endif  
