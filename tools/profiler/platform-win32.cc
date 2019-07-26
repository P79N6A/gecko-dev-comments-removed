



























#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include "platform.h"
#include "TableTicker.h"
#include "ProfileEntry.h"

class PlatformData : public Malloced {
 public:
  
  
  
  
  
  PlatformData(int aThreadId) : profiled_thread_(OpenThread(THREAD_GET_CONTEXT |
                                               THREAD_SUSPEND_RESUME |
                                               THREAD_QUERY_INFORMATION,
                                               false,
                                               aThreadId)) {}

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

 PlatformData*
Sampler::AllocPlatformData(int aThreadId)
{
  return new PlatformData(aThreadId);
}

 void
Sampler::FreePlatformData(PlatformData* aData)
{
  delete aData;
}

uintptr_t
Sampler::GetThreadHandle(PlatformData* aData)
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
      std::vector<ThreadInfo*> threads =
        sampler_->GetRegisteredThreads();
      for (uint32_t i = 0; i < threads.size(); i++) {
        ThreadInfo* info = threads[i];

        
        if (!info->Profile())
          continue;

        ThreadProfile* thread_profile = info->Profile();

        if (!sampler_->IsPaused()) {
          SampleContext(sampler_, thread_profile);
        }
      }
      OS::Sleep(interval_);
    }

    
    if (interval_ < 10)
        ::timeEndPeriod(interval_);
  }

  void SampleContext(Sampler* sampler, ThreadProfile* thread_profile) {
    uintptr_t thread = Sampler::GetThreadHandle(
                               thread_profile->GetPlatformData());
    HANDLE profiled_thread = reinterpret_cast<HANDLE>(thread);
    if (profiled_thread == NULL)
      return;

    
    CONTEXT context;
    memset(&context, 0, sizeof(context));

    TickSample sample_obj;
    TickSample* sample = &sample_obj;

    
    sample->timestamp = mozilla::TimeStamp::Now();
    sample->threadProfile = thread_profile;

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
      entrySize_(entrySize) {
}

Sampler::~Sampler() {
  ASSERT(!IsActive());
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



Thread::Thread(const char* name)
    : stack_size_(0) {
  thread_ = kNoThread;
  set_name(name);
}

void Thread::set_name(const char* name) {
  strncpy(name_, name, sizeof(name_));
  name_[sizeof(name_) - 1] = '\0';
}


Thread::~Thread() {
  if (thread_ != kNoThread) CloseHandle(thread_);
}




void Thread::Start() {
  thread_ = reinterpret_cast<HANDLE>(
      _beginthreadex(NULL,
                     static_cast<unsigned>(stack_size_),
                     ThreadEntry,
                     this,
                     0,
                     &thread_id_));
}


void Thread::Join() {
  if (thread_id_ != GetCurrentThreadId()) {
    WaitForSingleObject(thread_, INFINITE);
  }
}

void OS::Sleep(int milliseconds) {
  ::Sleep(milliseconds);
}

bool Sampler::RegisterCurrentThread(const char* aName, PseudoStack* aPseudoStack, bool aIsMainThread)
{
  mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

  ThreadInfo* info = new ThreadInfo(aName, GetCurrentThreadId(),
    aIsMainThread, aPseudoStack);

  bool profileThread = sActiveSampler &&
    (aIsMainThread || sActiveSampler->ProfileThreads());

  if (profileThread) {
    
    info->SetProfile(new ThreadProfile(info->Name(),
                                       sActiveSampler->EntrySize(),
                                       info->Stack(),
                                       GetCurrentThreadId(),
                                       info->GetPlatformData(),
                                       aIsMainThread));
    if (sActiveSampler->ProfileJS()) {
      info->Profile()->GetPseudoStack()->enableJSSampling();
    }
  }

  sRegisteredThreads->push_back(info);
  return true;
}

void Sampler::UnregisterCurrentThread()
{
  mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

  int id = GetCurrentThreadId();

  for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
    ThreadInfo* info = sRegisteredThreads->at(i);
    if (info->ThreadId() == id) {
      delete info;
      sRegisteredThreads->erase(sRegisteredThreads->begin() + i);
      break;
    }
  }
}
