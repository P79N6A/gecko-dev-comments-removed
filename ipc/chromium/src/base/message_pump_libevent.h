



#ifndef BASE_MESSAGE_PUMP_LIBEVENT_H_
#define BASE_MESSAGE_PUMP_LIBEVENT_H_

#include "base/message_pump.h"
#include "base/time.h"
#include "nsAutoPtr.h"


struct event_base;
struct event;

class nsDependentCSubstring;

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


  
  
  
  
  
  
  
  
  class SignalEvent {
     friend class MessagePumpLibevent;

  public:
    SignalEvent();
    ~SignalEvent();             

    
    bool StopCatching();

  private:
    void Init(event* e);
    event* ReleaseEvent();

    event* event_;

    DISALLOW_COPY_AND_ASSIGN(SignalEvent);
  };

  class SignalWatcher {
  public:
    virtual ~SignalWatcher() {}
    
    
    virtual void OnSignal(int sig) = 0;
  };

  
  
  
  
  
  bool CatchSignal(int sig,
                   SignalEvent* sigevent,
                   SignalWatcher* delegate);


  
  virtual void Run(Delegate* delegate);
  virtual void Quit();
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

 protected:

  virtual ~MessagePumpLibevent();

 private:

  
  bool Init();

  
  bool keep_running_;

  
  bool in_run_;

  
  TimeTicks delayed_work_time_;

  
  
  event_base* event_base_;

  
  static void OnLibeventNotification(int fd, short flags,
                                     void* context);

  
  static void OnLibeventSignalNotification(int sig, short flags,
                                           void* context);

  
  
  static void OnWakeup(int socket, short flags, void* context);
  
  int wakeup_pipe_in_;
  
  int wakeup_pipe_out_;
  
  event* wakeup_event_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpLibevent);
};





class LineWatcher : public MessagePumpLibevent::Watcher
{
public:
  LineWatcher(char aTerminator, int aBufferSize) : mReceivedIndex(0),
    mBufferSize(aBufferSize),
    mTerminator(aTerminator)
  {
    mReceiveBuffer = new char[mBufferSize];
  }

  ~LineWatcher() {}

protected:
  



  virtual void OnError() {}
  virtual void OnLineRead(int aFd, nsDependentCSubstring& aMessage) = 0;
  virtual void OnFileCanWriteWithoutBlocking(int ) {}
private:
  virtual void OnFileCanReadWithoutBlocking(int aFd) MOZ_FINAL;

  nsAutoPtr<char> mReceiveBuffer;
  int mReceivedIndex;
  int mBufferSize;
  char mTerminator;
};
}  

#endif  
