











#ifndef WEBRTC_BASE_FAKETASKRUNNER_H_
#define WEBRTC_BASE_FAKETASKRUNNER_H_

#include "webrtc/base/taskparent.h"
#include "webrtc/base/taskrunner.h"

namespace rtc {

class FakeTaskRunner : public TaskRunner {
 public:
  FakeTaskRunner() : current_time_(0) {}
  virtual ~FakeTaskRunner() {}

  virtual void WakeTasks() { RunTasks(); }

  virtual int64 CurrentTime() {
    
    return current_time_++;
  }

  int64 current_time_;
};

}  

#endif  
