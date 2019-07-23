



#ifndef BASE_MESSAGE_PUMP_LIBEVENT_H_
#define BASE_MESSAGE_PUMP_LIBEVENT_H_

#include "base/message_pump.h"
#include "base/time.h"


struct event_base;
struct event;

namespace base {



class MessagePumpLibevent : public MessagePump {
 public:

  
  class FileDescriptorWatcher {
    public:
     FileDescriptorWatcher();
     ~FileDescriptorWatcher();  

     
     

     
     
     bool StopWatchingFileDescriptor();

    private:
     
     
     void Init(event* e, bool is_persistent);

     
     event *ReleaseEvent();
     friend class MessagePumpLibevent;

    private:
     bool is_persistent_;  
     event* event_;
     DISALLOW_COPY_AND_ASSIGN(FileDescriptorWatcher);
  };

  
  
  class Watcher {
   public:
    virtual ~Watcher() {}
    
    
    virtual void OnFileCanReadWithoutBlocking(int fd) = 0;
    virtual void OnFileCanWriteWithoutBlocking(int fd) = 0;
  };

  MessagePumpLibevent();
  virtual ~MessagePumpLibevent();

  enum Mode {
    WATCH_READ = 1 << 0,
    WATCH_WRITE = 1 << 1,
    WATCH_READ_WRITE = WATCH_READ | WATCH_WRITE
  };

  
  
  
  
  
  
  
  
  
  
  
  bool WatchFileDescriptor(int fd,
                           bool persistent,
                           Mode mode,
                           FileDescriptorWatcher *controller,
                           Watcher *delegate);

  
  virtual void Run(Delegate* delegate);
  virtual void Quit();
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const Time& delayed_work_time);

 private:

  
  bool Init();

  
  bool keep_running_;

  
  bool in_run_;

  
  Time delayed_work_time_;

  
  
  event_base* event_base_;

  
  static void OnLibeventNotification(int fd, short flags,
                                     void* context);

  
  
  static void OnWakeup(int socket, short flags, void* context);
  
  int wakeup_pipe_in_;
  
  int wakeup_pipe_out_;
  
  event* wakeup_event_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpLibevent);
};

}  

#endif  
