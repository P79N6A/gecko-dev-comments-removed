



#include "chrome/common/child_process.h"

#include "base/basictypes.h"
#include "base/string_util.h"
#include "chrome/common/child_thread.h"

ChildProcess* ChildProcess::child_process_;

ChildProcess::ChildProcess(ChildThread* child_thread)
    : child_thread_(child_thread),
      ref_count_(0),
      shutdown_event_(true, false) {
  DCHECK(!child_process_);
  child_process_ = this;
  if (child_thread_.get())  
    child_thread_->Run();
}

ChildProcess::~ChildProcess() {
  DCHECK(child_process_ == this);

  
  
  
  
  shutdown_event_.Signal();

  if (child_thread_.get())
    child_thread_->Stop();

  child_process_ = NULL;
}

void ChildProcess::AddRefProcess() {
  DCHECK(!child_thread_.get() ||  
         MessageLoop::current() == child_thread_->message_loop());
  ref_count_++;
}

void ChildProcess::ReleaseProcess() {
  DCHECK(!child_thread_.get() ||  
         MessageLoop::current() == child_thread_->message_loop());
  DCHECK(ref_count_);
  DCHECK(child_process_);
  if (--ref_count_)
    return;

  if (child_thread_.get())  
    child_thread_->OnProcessFinalRelease();
}

base::WaitableEvent* ChildProcess::GetShutDownEvent() {
  DCHECK(child_process_);
  return &child_process_->shutdown_event_;
}
