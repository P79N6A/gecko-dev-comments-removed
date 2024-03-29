









#include "webrtc/base/task.h"
#include "webrtc/base/common.h"
#include "webrtc/base/taskrunner.h"

namespace rtc {

int32 Task::unique_id_seed_ = 0;

Task::Task(TaskParent *parent)
    : TaskParent(this, parent),
      state_(STATE_INIT),
      blocked_(false),
      done_(false),
      aborted_(false),
      busy_(false),
      error_(false),
      start_time_(0),
      timeout_time_(0),
      timeout_seconds_(0),
      timeout_suspended_(false)  {
  unique_id_ = unique_id_seed_++;

  
  ASSERT(unique_id_ < unique_id_seed_);
}

Task::~Task() {
  
  ASSERT(!done_ || GetRunner()->is_ok_to_delete(this));
  ASSERT(state_ == STATE_INIT || done_);
  ASSERT(state_ == STATE_INIT || blocked_);

  
  
  
  if (!done_) {
    Stop();
  }
}

int64 Task::CurrentTime() {
  return GetRunner()->CurrentTime();
}

int64 Task::ElapsedTime() {
  return CurrentTime() - start_time_;
}

void Task::Start() {
  if (state_ != STATE_INIT)
    return;
  
  
  
  start_time_ = CurrentTime();
  GetRunner()->StartTask(this);
}

void Task::Step() {
  if (done_) {
#ifdef _DEBUG
    
    
    
    ASSERT(blocked_);
#else
    blocked_ = true;
#endif
    return;
  }

  
  if (error_) {
    done_ = true;
    state_ = STATE_ERROR;
    blocked_ = true;



    Stop();
#ifdef _DEBUG
    
    ASSERT(!parent()->IsChildTask(this));
#endif
    return;
  }

  busy_ = true;
  int new_state = Process(state_);
  busy_ = false;

  if (aborted_) {
    Abort(true);  
    return;
  }

  if (new_state == STATE_BLOCKED) {
    blocked_ = true;
    
  } else {
    state_ = new_state;
    blocked_ = false;
    ResetTimeout();
  }

  if (new_state == STATE_DONE) {
    done_ = true;
  } else if (new_state == STATE_ERROR) {
    done_ = true;
    error_ = true;
  }

  if (done_) {



    Stop();
#if _DEBUG
    
    ASSERT(!parent()->IsChildTask(this));
#endif
    blocked_ = true;
  }
}

void Task::Abort(bool nowake) {
  
  
  
  
  
  if (done_)
    return;
  aborted_ = true;
  if (!busy_) {
    done_ = true;
    blocked_ = true;
    error_ = true;

    
    
    Stop();
#ifdef _DEBUG
    
    ASSERT(!parent()->IsChildTask(this));
#endif
    if (!nowake) {
      
      
      
      GetRunner()->WakeTasks();
    }
  }
}

void Task::Wake() {
  if (done_)
    return;
  if (blocked_) {
    blocked_ = false;
    GetRunner()->WakeTasks();
  }
}

void Task::Error() {
  if (error_ || done_)
    return;
  error_ = true;
  Wake();
}

std::string Task::GetStateName(int state) const {
  switch (state) {
    case STATE_BLOCKED: return "BLOCKED";
    case STATE_INIT: return "INIT";
    case STATE_START: return "START";
    case STATE_DONE: return "DONE";
    case STATE_ERROR: return "ERROR";
    case STATE_RESPONSE: return "RESPONSE";
  }
  return "??";
}

int Task::Process(int state) {
  int newstate = STATE_ERROR;

  if (TimedOut()) {
    ClearTimeout();
    newstate = OnTimeout();
    SignalTimeout();
  } else {
    switch (state) {
      case STATE_INIT:
        newstate = STATE_START;
        break;
      case STATE_START:
        newstate = ProcessStart();
        break;
      case STATE_RESPONSE:
        newstate = ProcessResponse();
        break;
      case STATE_DONE:
      case STATE_ERROR:
        newstate = STATE_BLOCKED;
        break;
    }
  }

  return newstate;
}

void Task::Stop() {
  
  TaskParent::OnStopped(this);
}

void Task::set_timeout_seconds(const int timeout_seconds) {
  timeout_seconds_ = timeout_seconds;
  ResetTimeout();
}

bool Task::TimedOut() {
  return timeout_seconds_ &&
    timeout_time_ &&
    CurrentTime() >= timeout_time_;
}

void Task::ResetTimeout() {
  int64 previous_timeout_time = timeout_time_;
  bool timeout_allowed = (state_ != STATE_INIT)
                      && (state_ != STATE_DONE)
                      && (state_ != STATE_ERROR);
  if (timeout_seconds_ && timeout_allowed && !timeout_suspended_)
    timeout_time_ = CurrentTime() +
                    (timeout_seconds_ * kSecToMsec * kMsecTo100ns);
  else
    timeout_time_ = 0;

  GetRunner()->UpdateTaskTimeout(this, previous_timeout_time);
}

void Task::ClearTimeout() {
  int64 previous_timeout_time = timeout_time_;
  timeout_time_ = 0;
  GetRunner()->UpdateTaskTimeout(this, previous_timeout_time);
}

void Task::SuspendTimeout() {
  if (!timeout_suspended_) {
    timeout_suspended_ = true;
    ResetTimeout();
  }
}

void Task::ResumeTimeout() {
  if (timeout_suspended_) {
    timeout_suspended_ = false;
    ResetTimeout();
  }
}

} 
