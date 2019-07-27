









#include "webrtc/base/signalthread.h"

#include "webrtc/base/common.h"

namespace rtc {





SignalThread::SignalThread()
    : main_(Thread::Current()),
      worker_(this),
      state_(kInit),
      refcount_(1) {
  main_->SignalQueueDestroyed.connect(this,
                                      &SignalThread::OnMainThreadDestroyed);
  worker_.SetName("SignalThread", this);
}

SignalThread::~SignalThread() {
  ASSERT(refcount_ == 0);
}

bool SignalThread::SetName(const std::string& name, const void* obj) {
  EnterExit ee(this);
  ASSERT(main_->IsCurrent());
  ASSERT(kInit == state_);
  return worker_.SetName(name, obj);
}

bool SignalThread::SetPriority(ThreadPriority priority) {
  EnterExit ee(this);
  ASSERT(main_->IsCurrent());
  ASSERT(kInit == state_);
  return worker_.SetPriority(priority);
}

void SignalThread::Start() {
  EnterExit ee(this);
  ASSERT(main_->IsCurrent());
  if (kInit == state_ || kComplete == state_) {
    state_ = kRunning;
    OnWorkStart();
    worker_.Start();
  } else {
    ASSERT(false);
  }
}

void SignalThread::Destroy(bool wait) {
  EnterExit ee(this);
  ASSERT(main_->IsCurrent());
  if ((kInit == state_) || (kComplete == state_)) {
    refcount_--;
  } else if (kRunning == state_ || kReleasing == state_) {
    state_ = kStopping;
    
    
    worker_.Quit();
    OnWorkStop();
    if (wait) {
      
      cs_.Leave();
      worker_.Stop();
      cs_.Enter();
      refcount_--;
    }
  } else {
    ASSERT(false);
  }
}

void SignalThread::Release() {
  EnterExit ee(this);
  ASSERT(main_->IsCurrent());
  if (kComplete == state_) {
    refcount_--;
  } else if (kRunning == state_) {
    state_ = kReleasing;
  } else {
    
    ASSERT(false);
  }
}

bool SignalThread::ContinueWork() {
  EnterExit ee(this);
  ASSERT(worker_.IsCurrent());
  return worker_.ProcessMessages(0);
}

void SignalThread::OnMessage(Message *msg) {
  EnterExit ee(this);
  if (ST_MSG_WORKER_DONE == msg->message_id) {
    ASSERT(main_->IsCurrent());
    OnWorkDone();
    bool do_delete = false;
    if (kRunning == state_) {
      state_ = kComplete;
    } else {
      do_delete = true;
    }
    if (kStopping != state_) {
      
      
      
      
      
      
      
      
      
      
      worker_.Stop();
      SignalWorkDone(this);
    }
    if (do_delete) {
      refcount_--;
    }
  }
}

void SignalThread::Run() {
  DoWork();
  {
    EnterExit ee(this);
    if (main_) {
      main_->Post(this, ST_MSG_WORKER_DONE);
    }
  }
}

void SignalThread::OnMainThreadDestroyed() {
  EnterExit ee(this);
  main_ = NULL;
}

}  
