









#include "webrtc/base/worker.h"

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/thread.h"

namespace rtc {

enum {
  MSG_HAVEWORK = 0,
};

Worker::Worker() : worker_thread_(NULL) {}

Worker::~Worker() {
  
  
  
  ASSERT(!worker_thread_);
}

bool Worker::StartWork() {
  rtc::Thread *me = rtc::Thread::Current();
  if (worker_thread_) {
    if (worker_thread_ == me) {
      
      return true;
    } else {
      LOG(LS_ERROR) << "Automatically switching threads is not supported";
      ASSERT(false);
      return false;
    }
  }
  worker_thread_ = me;
  OnStart();
  return true;
}

bool Worker::StopWork() {
  if (!worker_thread_) {
    
    return true;
  } else if (worker_thread_ != rtc::Thread::Current()) {
    LOG(LS_ERROR) << "Stopping from a different thread is not supported";
    ASSERT(false);
    return false;
  }
  OnStop();
  worker_thread_->Clear(this, MSG_HAVEWORK);
  worker_thread_ = NULL;
  return true;
}

void Worker::HaveWork() {
  ASSERT(worker_thread_ != NULL);
  worker_thread_->Post(this, MSG_HAVEWORK);
}

void Worker::OnMessage(rtc::Message *msg) {
  ASSERT(msg->message_id == MSG_HAVEWORK);
  ASSERT(worker_thread_ == rtc::Thread::Current());
  OnHaveWork();
}

}  
