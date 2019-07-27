









#ifndef WEBRTC_BASE_TASK_H__
#define WEBRTC_BASE_TASK_H__

#include <string>
#include "webrtc/base/basictypes.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/taskparent.h"







































































namespace rtc {


class Task : public TaskParent {
 public:
  Task(TaskParent *parent);
  virtual ~Task();

  int32 unique_id() { return unique_id_; }

  void Start();
  void Step();
  int GetState() const { return state_; }
  bool HasError() const { return (GetState() == STATE_ERROR); }
  bool Blocked() const { return blocked_; }
  bool IsDone() const { return done_; }
  int64 ElapsedTime();

  
  void Abort(bool nowake = false);

  bool TimedOut();

  int64 timeout_time() const { return timeout_time_; }
  int timeout_seconds() const { return timeout_seconds_; }
  void set_timeout_seconds(int timeout_seconds);

  sigslot::signal0<> SignalTimeout;

  
  void Wake();

 protected:

  enum {
    STATE_BLOCKED = -1,
    STATE_INIT = 0,
    STATE_START = 1,
    STATE_DONE = 2,
    STATE_ERROR = 3,
    STATE_RESPONSE = 4,
    STATE_NEXT = 5,  
  };

  
  void Error();

  int64 CurrentTime();

  virtual std::string GetStateName(int state) const;
  virtual int Process(int state);
  virtual void Stop();
  virtual int ProcessStart() = 0;
  virtual int ProcessResponse() { return STATE_DONE; }

  void ResetTimeout();
  void ClearTimeout();

  void SuspendTimeout();
  void ResumeTimeout();

 protected:
  virtual int OnTimeout() {
    
    return STATE_DONE;
  }

 private:
  void Done();

  int state_;
  bool blocked_;
  bool done_;
  bool aborted_;
  bool busy_;
  bool error_;
  int64 start_time_;
  int64 timeout_time_;
  int timeout_seconds_;
  bool timeout_suspended_;
  int32 unique_id_;
  
  static int32 unique_id_seed_;
};

}  

#endif
