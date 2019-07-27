









#ifndef WEBRTC_BASE_SIGNALTHREAD_H_
#define WEBRTC_BASE_SIGNALTHREAD_H_

#include <string>

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/thread.h"

namespace rtc {


















class SignalThread
    : public sigslot::has_slots<>,
      protected MessageHandler {
 public:
  SignalThread();

  
  bool SetName(const std::string& name, const void* obj);

  
  bool SetPriority(ThreadPriority priority);

  
  void Start();

  
  
  
  
  
  void Destroy(bool wait);

  
  
  
  void Release();

  
  sigslot::signal1<SignalThread *> SignalWorkDone;

  enum { ST_MSG_WORKER_DONE, ST_MSG_FIRST_AVAILABLE };

 protected:
  virtual ~SignalThread();

  Thread* worker() { return &worker_; }

  
  virtual void OnWorkStart() { }

  
  virtual void DoWork() = 0;

  
  
  bool ContinueWork();

  
  
  virtual void OnWorkStop() { }

  
  virtual void OnWorkDone() { }

  
  
  virtual void OnMessage(Message *msg);

 private:
  enum State {
    kInit,            
    kRunning,         
    kReleasing,       
    kComplete,        
    kStopping,        
  };

  class Worker : public Thread {
   public:
    explicit Worker(SignalThread* parent) : parent_(parent) {}
    virtual ~Worker() { Stop(); }
    virtual void Run() { parent_->Run(); }

   private:
    SignalThread* parent_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(Worker);
  };

  class SCOPED_LOCKABLE EnterExit {
   public:
    explicit EnterExit(SignalThread* t) EXCLUSIVE_LOCK_FUNCTION(t->cs_)
        : t_(t) {
      t_->cs_.Enter();
      
      
      ASSERT(t_->refcount_ != 0);
      ++t_->refcount_;
    }
    ~EnterExit() UNLOCK_FUNCTION() {
      bool d = (0 == --t_->refcount_);
      t_->cs_.Leave();
      if (d)
        delete t_;
    }

   private:
    SignalThread* t_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(EnterExit);
  };

  void Run();
  void OnMainThreadDestroyed();

  Thread* main_;
  Worker worker_;
  CriticalSection cs_;
  State state_;
  int refcount_;

  DISALLOW_COPY_AND_ASSIGN(SignalThread);
};



}  

#endif
