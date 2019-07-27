





#ifndef dom_plugins_IOThreadChild_h
#define dom_plugins_IOThreadChild_h 1

#include "chrome/common/child_thread.h"

namespace mozilla {
namespace ipc {




class IOThreadChild : public ChildThread {
public:
  IOThreadChild()
    : ChildThread(base::Thread::Options(MessageLoop::TYPE_IO,
                                        0)) 
  { }

  ~IOThreadChild()
  { }

  static MessageLoop* message_loop() {
    return IOThreadChild::current()->Thread::message_loop();
  }

  
  static IPC::Channel* channel() {
    return IOThreadChild::current()->ChildThread::channel();
  }

protected:
  static IOThreadChild* current() {
    return static_cast<IOThreadChild*>(ChildThread::current());
  }

private:
  DISALLOW_EVIL_CONSTRUCTORS(IOThreadChild);
};

} 
} 

#endif  
