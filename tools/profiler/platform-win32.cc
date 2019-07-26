



























#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include "platform.h"
#include "ProfileEntry.h"

class Sampler::PlatformData : public Malloced {
 public:
  
  
  
  
  
  PlatformData() : profiled_thread_(OpenThread(THREAD_GET_CONTEXT |
                                               THREAD_SUSPEND_RESUME |
                                               THREAD_QUERY_INFORMATION,
                                               false,
                                               GetCurrentThreadId())) {}

  ~PlatformData() {
    if (profiled_thread_ != NULL) {
      CloseHandle(profiled_thread_);
      profiled_thread_ = NULL;
    }
  }

  HANDLE profiled_thread() { return profiled_thread_; }

 private:
  HANDLE profiled_thread_;
};

uintptr_t
Sampler::GetThreadHandle(Sampler::PlatformData* aData)
{
  return (uintptr_t) aData->profiled_thread();
}

class SamplerThread : public Thread {
 public:
  SamplerThread(int interval, Sampler* sampler)
      : Thread("SamplerThread")
      , interval_(interval)
      , sampler_(sampler) {}

  static void StartSampler(Sampler* sampler) {
    if (instance_ == NULL) {
      instance_ = new SamplerThread(sampler->interval(), sampler);
      instance_->Start();
    } else {
      ASSERT(instance_->interval_ == sampler->interval());
    }
  } 

  static void StopSampler() {
    instance_->Join();
    delete instance_;
    instance_ = NULL;
  }

  
  virtual void Run() {

    
    
    
    if (interval_ < 10)
        ::timeBeginPeriod(interval_);

    while (sampler_->IsActive()) {
      if (!sampler_->IsPaused())
        SampleContext(sampler_);
      OS::Sleep(interval_);
    }

    
    if (interval_ < 10)
        ::timeEndPeriod(interval_);
  }

  void SampleContext(Sampler* sampler) {
    HANDLE profiled_thread = sampler->platform_data()->profiled_thread();
    if (profiled_thread == NULL)
      return;

    
    CONTEXT context;
    memset(&context, 0, sizeof(context));

    TickSample sample_obj;
    TickSample* sample = &sample_obj;

    
    sample->timestamp = mozilla::TimeStamp::Now();
    sample->threadProfile = NULL;

    static const DWORD kSuspendFailed = static_cast<DWORD>(-1);
    if (SuspendThread(profiled_thread) == kSuspendFailed)
      return;

    context.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(profiled_thread, &context) != 0) {
#if V8_HOST_ARCH_X64
      sample->pc = reinterpret_cast<Address>(context.Rip);
      sample->sp = reinterpret_cast<Address>(context.Rsp);
      sample->fp = reinterpret_cast<Address>(context.Rbp);
#else
      sample->pc = reinterpret_cast<Address>(context.Eip);
      sample->sp = reinterpret_cast<Address>(context.Esp);
      sample->fp = reinterpret_cast<Address>(context.Ebp);
#endif
      sample->context = &context;
      sampler->SampleStack(sample);
      sampler->Tick(sample);
    }
    ResumeThread(profiled_thread);
  }

  Sampler* sampler_;
  const int interval_;

  
  static SamplerThread* instance_;

  DISALLOW_COPY_AND_ASSIGN(SamplerThread);
};

SamplerThread* SamplerThread::instance_ = NULL;


Sampler::Sampler(int interval, bool profiling, int entrySize)
    : interval_(interval),
      profiling_(profiling),
      paused_(false),
      active_(false),
      entrySize_(entrySize),
      data_(new PlatformData) {
}

Sampler::~Sampler() {
  ASSERT(!IsActive());
  delete data_;
}

void Sampler::Start() {
  ASSERT(!IsActive());
  SetActive(true);
  SamplerThread::StartSampler(this);
}

void Sampler::Stop() {
  ASSERT(IsActive());
  SetActive(false);
  SamplerThread::StopSampler();
}


static const HANDLE kNoThread = INVALID_HANDLE_VALUE;

static unsigned int __stdcall ThreadEntry(void* arg) {
  Thread* thread = reinterpret_cast<Thread*>(arg);
  thread->Run();
  return 0;
}

class Thread::PlatformData : public Malloced {
 public:
  explicit PlatformData(HANDLE thread) : thread_(thread) {}
  HANDLE thread_;
  unsigned thread_id_;
};



Thread::Thread(const char* name)
    : stack_size_(0) {
  data_ = new PlatformData(kNoThread);
  set_name(name);
}

void Thread::set_name(const char* name) {
  strncpy(name_, name, sizeof(name_));
  name_[sizeof(name_) - 1] = '\0';
}


Thread::~Thread() {
  if (data_->thread_ != kNoThread) CloseHandle(data_->thread_);
  delete data_;
}




void Thread::Start() {
  data_->thread_ = reinterpret_cast<HANDLE>(
      _beginthreadex(NULL,
                     static_cast<unsigned>(stack_size_),
                     ThreadEntry,
                     this,
                     0,
                     &data_->thread_id_));
}


void Thread::Join() {
  if (data_->thread_id_ != GetCurrentThreadId()) {
    WaitForSingleObject(data_->thread_, INFINITE);
  }
}

void OS::Sleep(int milliseconds) {
  ::Sleep(milliseconds);
}

bool Sampler::RegisterCurrentThread(const char* aName, PseudoStack* aPseudoStack, bool aIsMainThread)
{
  mozilla::MutexAutoLock lock(sRegisteredThreadsMutex);

  if (!aIsMainThread)
    return false;

  ThreadInfo* info = new ThreadInfo(aName, 0, true, aPseudoStack);

  if (sActiveSampler) {
    
    info->SetProfile(new ThreadProfile(info->Name(),
                                       sActiveSampler->EntrySize(),
                                       info->Stack(),
                                       info->ThreadId(),
                                       true));
  }

  sRegisteredThreads.push_back(info);
  return true;
}

void Sampler::UnregisterCurrentThread()
{
  
}
