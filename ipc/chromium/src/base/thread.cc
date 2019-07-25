



#include "base/thread.h"

#include "base/lazy_instance.h"
#include "base/string_util.h"
#include "base/thread_local.h"
#include "base/waitable_event.h"

namespace base {


class ThreadQuitTask : public Task {
 public:
  virtual void Run() {
    MessageLoop::current()->Quit();
    Thread::SetThreadWasQuitProperly(true);
  }
};



struct Thread::StartupData {
  
  const Thread::Options& options;

  
  WaitableEvent event;

  explicit StartupData(const Options& opt)
      : options(opt),
        event(false, false) {}
};

Thread::Thread(const char *name)
    : startup_data_(NULL),
      thread_(0),
      message_loop_(NULL),
      thread_id_(0),
      name_(name) {
}

Thread::~Thread() {
  Stop();
}

namespace {





base::LazyInstance<base::ThreadLocalBoolean> lazy_tls_bool(
    base::LINKER_INITIALIZED);

}  

void Thread::SetThreadWasQuitProperly(bool flag) {
  lazy_tls_bool.Pointer()->Set(flag);
}

bool Thread::GetThreadWasQuitProperly() {
  bool quit_properly = true;
#ifndef NDEBUG
  quit_properly = lazy_tls_bool.Pointer()->Get();
#endif
  return quit_properly;
}

bool Thread::Start() {
  return StartWithOptions(Options());
}

bool Thread::StartWithOptions(const Options& options) {
  DCHECK(!message_loop_);

  SetThreadWasQuitProperly(false);

  StartupData startup_data(options);
  startup_data_ = &startup_data;

  if (!PlatformThread::Create(options.stack_size, this, &thread_)) {
    DLOG(ERROR) << "failed to create thread";
    startup_data_ = NULL;  
    return false;
  }

  
  startup_data.event.Wait();

  DCHECK(message_loop_);
  return true;
}

void Thread::Stop() {
  if (!thread_was_started())
    return;

  
  DCHECK_NE(thread_id_, PlatformThread::CurrentId());

  
  if (message_loop_)
    message_loop_->PostTask(FROM_HERE, new ThreadQuitTask());

  
  
  
  
  
  
  PlatformThread::Join(thread_);

  
  message_loop_ = NULL;

  
  startup_data_ = NULL;
}

void Thread::StopSoon() {
  if (!message_loop_)
    return;

  
  DCHECK_NE(thread_id_, PlatformThread::CurrentId());

  
  
  
  DCHECK(message_loop_);

  message_loop_->PostTask(FROM_HERE, new ThreadQuitTask());
}

void Thread::ThreadMain() {
  
  MessageLoop message_loop(startup_data_->options.message_loop_type);

  
  thread_id_ = PlatformThread::CurrentId();
  PlatformThread::SetName(name_.c_str());
  message_loop.set_thread_name(name_);
  message_loop_ = &message_loop;

  
  
  Init();

  startup_data_->event.Signal();
  
  

  message_loop.Run();

  
  CleanUp();

  
  DCHECK(GetThreadWasQuitProperly());

  
  message_loop_ = NULL;
  thread_id_ = 0;
}

}  
