









#ifndef WEBRTC_BASE_THREAD_H_
#define WEBRTC_BASE_THREAD_H_

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#if defined(WEBRTC_POSIX)
#include <pthread.h>
#endif
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/event.h"
#include "webrtc/base/messagequeue.h"

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#endif

namespace rtc {

class Thread;

class ThreadManager {
 public:
  ThreadManager();
  ~ThreadManager();

  static ThreadManager* Instance();

  Thread* CurrentThread();
  void SetCurrentThread(Thread* thread);

  
  
  
  
  
  
  
  
  
  
  
  
  
  Thread *WrapCurrentThread();
  void UnwrapCurrentThread();

 private:
#if defined(WEBRTC_POSIX)
  pthread_key_t key_;
#endif

#if defined(WEBRTC_WIN)
  DWORD key_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ThreadManager);
};

struct _SendMessage {
  _SendMessage() {}
  Thread *thread;
  Message msg;
  bool *ready;
};

enum ThreadPriority {
  PRIORITY_IDLE = -1,
  PRIORITY_NORMAL = 0,
  PRIORITY_ABOVE_NORMAL = 1,
  PRIORITY_HIGH = 2,
};

class Runnable {
 public:
  virtual ~Runnable() {}
  virtual void Run(Thread* thread) = 0;

 protected:
  Runnable() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Runnable);
};



class Thread : public MessageQueue {
 public:
  explicit Thread(SocketServer* ss = NULL);
  
  
  
  
  virtual ~Thread();

  static Thread* Current();

  
  
  
  
  class ScopedDisallowBlockingCalls {
   public:
    ScopedDisallowBlockingCalls();
    ~ScopedDisallowBlockingCalls();
   private:
    Thread* const thread_;
    const bool previous_state_;
  };

  bool IsCurrent() const {
    return Current() == this;
  }

  
  
  
  static bool SleepMs(int millis);

  
  
  const std::string& name() const { return name_; }
  bool SetName(const std::string& name, const void* obj);

  
  ThreadPriority priority() const { return priority_; }
  bool SetPriority(ThreadPriority priority);

  
  bool Start(Runnable* runnable = NULL);

  
  
  
  
  virtual void Stop();

  
  
  
  virtual void Run();

  virtual void Send(MessageHandler *phandler, uint32 id = 0,
      MessageData *pdata = NULL);

  
  
  
  
  
  
  
  template <class ReturnT, class FunctorT>
  ReturnT Invoke(const FunctorT& functor) {
    FunctorMessageHandler<ReturnT, FunctorT> handler(functor);
    Send(&handler);
    return handler.result();
  }

  
  virtual void Clear(MessageHandler *phandler, uint32 id = MQID_ANY,
                     MessageList* removed = NULL);
  virtual void ReceiveSends();

  
  
  
  bool ProcessMessages(int cms);

  
  
  
  
  
  
  bool IsOwned();

#if defined(WEBRTC_WIN)
  HANDLE GetHandle() const {
    return thread_;
  }
  DWORD GetId() const {
    return thread_id_;
  }
#elif defined(WEBRTC_POSIX)
  pthread_t GetPThread() {
    return thread_;
  }
#endif

  
  
  
  
  
  
  bool RunningForTest() { return running(); }

  
  
  bool SetAllowBlockingCalls(bool allow);

  
  
  
  
  
  
  bool WrapCurrent();
  void UnwrapCurrent();

 protected:
  
  
  
  void SafeWrapCurrent();

  
  void Join();

  static void AssertBlockingIsAllowedOnCurrentThread();

  friend class ScopedDisallowBlockingCalls;

 private:
  static void *PreRun(void *pv);

  
  
  
  
  
  bool WrapCurrentWithThreadManager(ThreadManager* thread_manager,
                                    bool need_synchronize_access);

  
  bool running() { return running_.Wait(0); }

  
  
  void ReceiveSendsFromThread(const Thread* source);

  
  
  
  
  bool PopSendMessageFromThread(const Thread* source, _SendMessage* msg);

  std::list<_SendMessage> sendlist_;
  std::string name_;
  ThreadPriority priority_;
  Event running_;  

#if defined(WEBRTC_POSIX)
  pthread_t thread_;
#endif

#if defined(WEBRTC_WIN)
  HANDLE thread_;
  DWORD thread_id_;
#endif

  bool owned_;
  bool blocking_calls_allowed_;  

  friend class ThreadManager;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};





class AutoThread : public Thread {
 public:
  explicit AutoThread(SocketServer* ss = 0);
  virtual ~AutoThread();

 private:
  DISALLOW_COPY_AND_ASSIGN(AutoThread);
};


#if defined(WEBRTC_WIN)
class ComThread : public Thread {
 public:
  ComThread() {}
  virtual ~ComThread() { Stop(); }

 protected:
  virtual void Run();

 private:
  DISALLOW_COPY_AND_ASSIGN(ComThread);
};
#endif


class SocketServerScope {
 public:
  explicit SocketServerScope(SocketServer* ss) {
    old_ss_ = Thread::Current()->socketserver();
    Thread::Current()->set_socketserver(ss);
  }
  ~SocketServerScope() {
    Thread::Current()->set_socketserver(old_ss_);
  }

 private:
  SocketServer* old_ss_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(SocketServerScope);
};

}  

#endif  
