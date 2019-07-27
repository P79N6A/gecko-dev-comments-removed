



#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mach/mach_init.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>

#include <AvailabilityMacros.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <libkern/OSAtomic.h>
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <mach/task.h>
#include <mach/vm_statistics.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "ThreadResponsiveness.h"
#include "nsThreadUtils.h"

#include "platform.h"
#include "TableTicker.h"
#include "mozilla/TimeStamp.h"


#include "nsMemoryReporterManager.h"





struct SamplerRegistry {
  static void AddActiveSampler(Sampler *sampler) {
    ASSERT(!SamplerRegistry::sampler);
    SamplerRegistry::sampler = sampler;
  }
  static void RemoveActiveSampler(Sampler *sampler) {
    SamplerRegistry::sampler = NULL;
  }
  static Sampler *sampler;
};

Sampler *SamplerRegistry::sampler = NULL;









static mozilla::Atomic<ThreadProfile*> sCurrentThreadProfile;
static mozilla::Atomic<bool> sSignalHandlingDone;

#ifdef DEBUG

static const pthread_t kNoThread = (pthread_t) 0;
#endif

static void SetSampleContext(TickSample* sample, void* context)
{
  
  ucontext_t* ucontext = reinterpret_cast<ucontext_t*>(context);
  mcontext_t& mcontext = ucontext->uc_mcontext;
#if defined(SPS_PLAT_amd64_darwin)
  sample->pc = reinterpret_cast<Address>(mcontext->__ss.__rip);
  sample->sp = reinterpret_cast<Address>(mcontext->__ss.__rsp);
  sample->fp = reinterpret_cast<Address>(mcontext->__ss.__rbp);
#elif defined(SPS_PLAT_x86_darwin)
  sample->pc = reinterpret_cast<Address>(mcontext->__ss.__eip);
  sample->sp = reinterpret_cast<Address>(mcontext->__ss.__esp);
  sample->fp = reinterpret_cast<Address>(mcontext->__ss.__ebp);
#endif
}

void OS::Startup() {
}

void OS::Sleep(int milliseconds) {
  usleep(1000 * milliseconds);
}

void OS::SleepMicro(int microseconds) {
  usleep(microseconds);
}

Thread::Thread(const char* name)
    : stack_size_(0) {
  set_name(name);
}


Thread::~Thread() {
}


static void SetThreadName(const char* name) {
  
  
  int (*dynamic_pthread_setname_np)(const char*);
  *reinterpret_cast<void**>(&dynamic_pthread_setname_np) =
    dlsym(RTLD_DEFAULT, "pthread_setname_np");
  if (!dynamic_pthread_setname_np)
    return;

  
  static const int kMaxNameLength = 63;
  USE(kMaxNameLength);
  ASSERT(Thread::kMaxThreadNameLength <= kMaxNameLength);
  dynamic_pthread_setname_np(name);
}


static void* ThreadEntry(void* arg) {
  Thread* thread = reinterpret_cast<Thread*>(arg);

  thread->thread_ = pthread_self();
  SetThreadName(thread->name());
  ASSERT(thread->thread_ != kNoThread);
  thread->Run();
  return NULL;
}


void Thread::set_name(const char* name) {
  strncpy(name_, name, sizeof(name_));
  name_[sizeof(name_) - 1] = '\0';
}


void Thread::Start() {
  pthread_attr_t* attr_ptr = NULL;
  pthread_attr_t attr;
  if (stack_size_ > 0) {
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, static_cast<size_t>(stack_size_));
    attr_ptr = &attr;
  }
  pthread_create(&thread_, attr_ptr, ThreadEntry, this);
  ASSERT(thread_ != kNoThread);
}

void Thread::Join() {
  pthread_join(thread_, NULL);
}


namespace {

void ProfilerSignalHandler(int signal, siginfo_t* info, void* context) {
  if (!Sampler::GetActiveSampler()) {
    sSignalHandlingDone = true;
    return;
  }

  TickSample sample_obj;
  TickSample* sample = &sample_obj;
  sample->context = context;

  
  if (Sampler::GetActiveSampler()->IsProfiling()) {
    SetSampleContext(sample, context);
  }
  sample->threadProfile = sCurrentThreadProfile;
  sample->timestamp = mozilla::TimeStamp::Now();
  sample->rssMemory = sample->threadProfile->mRssMemory;
  sample->ussMemory = sample->threadProfile->mUssMemory;

  Sampler::GetActiveSampler()->Tick(sample);

  sCurrentThreadProfile = NULL;
  sSignalHandlingDone = true;
}

} 

static void ProfilerSignalThread(ThreadProfile *profile,
                                 bool isFirstProfiledThread)
{
  if (isFirstProfiledThread && Sampler::GetActiveSampler()->ProfileMemory()) {
    profile->mRssMemory = nsMemoryReporterManager::ResidentFast();
    profile->mUssMemory = nsMemoryReporterManager::ResidentUnique();
  } else {
    profile->mRssMemory = 0;
    profile->mUssMemory = 0;
  }
}

class PlatformData : public Malloced {
 public:
  PlatformData() : profiled_thread_(mach_thread_self())
  {
    profiled_pthread_ = pthread_from_mach_thread_np(profiled_thread_);
  }

  ~PlatformData() {
    
    mach_port_deallocate(mach_task_self(), profiled_thread_);
  }

  thread_act_t profiled_thread() { return profiled_thread_; }
  pthread_t profiled_pthread() { return profiled_pthread_; }

 private:
  
  
  
  thread_act_t profiled_thread_;
  
  
  
  pthread_t profiled_pthread_;
};

 PlatformData*
Sampler::AllocPlatformData(int aThreadId)
{
  return new PlatformData;
}

 void
Sampler::FreePlatformData(PlatformData* aData)
{
  delete aData;
}

class SamplerThread : public Thread {
 public:
  explicit SamplerThread(double interval)
      : Thread("SamplerThread")
      , intervalMicro_(floor(interval * 1000 + 0.5))
  {
    if (intervalMicro_ <= 0) {
      intervalMicro_ = 1;
    }
  }

  static void AddActiveSampler(Sampler* sampler) {
    SamplerRegistry::AddActiveSampler(sampler);
    if (instance_ == NULL) {
      instance_ = new SamplerThread(sampler->interval());
      instance_->Start();
    }
  }

  static void RemoveActiveSampler(Sampler* sampler) {
    instance_->Join();
    
    
    SamplerRegistry::RemoveActiveSampler(sampler);
    delete instance_;
    instance_ = NULL;
  }

  
  virtual void Run() {
    TimeDuration lastSleepOverhead = 0;
    TimeStamp sampleStart = TimeStamp::Now();
    while (SamplerRegistry::sampler->IsActive()) {
      SamplerRegistry::sampler->DeleteExpiredMarkers();
      if (!SamplerRegistry::sampler->IsPaused()) {
        mozilla::MutexAutoLock lock(*Sampler::sRegisteredThreadsMutex);
        std::vector<ThreadInfo*> threads =
          SamplerRegistry::sampler->GetRegisteredThreads();
        bool isFirstProfiledThread = true;
        for (uint32_t i = 0; i < threads.size(); i++) {
          ThreadInfo* info = threads[i];

          
          if (!info->Profile() || info->IsPendingDelete())
            continue;

          PseudoStack::SleepState sleeping = info->Stack()->observeSleeping();
          if (sleeping == PseudoStack::SLEEPING_AGAIN) {
            info->Profile()->DuplicateLastSample();
            continue;
          }

          info->Profile()->GetThreadResponsiveness()->Update();

          ThreadProfile* thread_profile = info->Profile();
          sCurrentThreadProfile = thread_profile;

          ProfilerSignalThread(sCurrentThreadProfile, isFirstProfiledThread);

          SampleContext(SamplerRegistry::sampler, thread_profile,
                        isFirstProfiledThread);
          isFirstProfiledThread = false;
        }
      }

      TimeStamp targetSleepEndTime = sampleStart + TimeDuration::FromMicroseconds(intervalMicro_);
      TimeStamp beforeSleep = TimeStamp::Now();
      TimeDuration targetSleepDuration = targetSleepEndTime - beforeSleep;
      double sleepTime = std::max(0.0, (targetSleepDuration - lastSleepOverhead).ToMicroseconds());
      OS::SleepMicro(sleepTime);
      sampleStart = TimeStamp::Now();
      lastSleepOverhead = sampleStart - (beforeSleep + TimeDuration::FromMicroseconds(sleepTime));
    }
  }

  void SampleContext(Sampler* sampler, ThreadProfile* thread_profile,
                     bool isFirstProfiledThread)
  {
    pthread_t profiled_pthread =
      thread_profile->GetPlatformData()->profiled_pthread();

    MOZ_ASSERT(sSignalHandlingDone == false);
    pthread_kill(profiled_pthread, SIGPROF);
    while (!sSignalHandlingDone) {
      sched_yield();
    }
    sSignalHandlingDone = false;
  }

  int intervalMicro_;
  

  static SamplerThread* instance_;

  DISALLOW_COPY_AND_ASSIGN(SamplerThread);
};

#undef REGISTER_FIELD

SamplerThread* SamplerThread::instance_ = NULL;

Sampler::Sampler(double interval, bool profiling, int entrySize)
    : 
      interval_(interval),
      profiling_(profiling),
      paused_(false),
      active_(false),
      entrySize_(entrySize) 
 {
}


Sampler::~Sampler() {
  ASSERT(!IsActive());
}


void Sampler::Start() {
  ASSERT(!IsActive());

  
  sCurrentThreadProfile = NULL;
  sSignalHandlingDone = false;

  
  LOG("Request signal");
  struct sigaction sa;
  sa.sa_sigaction = ProfilerSignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGPROF, &sa, &old_sigprof_signal_handler_) != 0) {
    LOG("Error installing signal");
    return;
  }
  signal_handler_installed_ = true;

  
  
  
  SetActive(true);
  SamplerThread::AddActiveSampler(this);
  LOG("Profiler thread started");
}


void Sampler::Stop() {
  ASSERT(IsActive());
  SetActive(false);
  SamplerThread::RemoveActiveSampler(this);

  
  if (signal_handler_installed_) {
    sigaction(SIGPROF, &old_sigprof_signal_handler_, 0);
    signal_handler_installed_ = false;
  }
}

pthread_t
Sampler::GetProfiledThread(PlatformData* aData)
{
  return aData->profiled_pthread();
}

#include <sys/syscall.h>
pid_t gettid()
{
  return (pid_t) syscall(SYS_thread_selfid);
}

 Thread::tid_t
Thread::GetCurrentId()
{
  return gettid();
}

bool Sampler::RegisterCurrentThread(const char* aName,
                                    PseudoStack* aPseudoStack,
                                    bool aIsMainThread, void* stackTop)
{
  if (!Sampler::sRegisteredThreadsMutex)
    return false;


  mozilla::MutexAutoLock lock(*Sampler::sRegisteredThreadsMutex);

  int id = gettid();
  for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
    ThreadInfo* info = sRegisteredThreads->at(i);
    if (info->ThreadId() == id && !info->IsPendingDelete()) {
      
      
      ASSERT(false);
      return false;
    }
  }

  set_tls_stack_top(stackTop);

  ThreadInfo* info = new StackOwningThreadInfo(aName, id,
    aIsMainThread, aPseudoStack, stackTop);

  if (sActiveSampler) {
    sActiveSampler->RegisterThread(info);
  }

  sRegisteredThreads->push_back(info);

  return true;
}

void Sampler::UnregisterCurrentThread()
{
  if (!Sampler::sRegisteredThreadsMutex)
    return;

  tlsStackTop.set(nullptr);

  mozilla::MutexAutoLock lock(*Sampler::sRegisteredThreadsMutex);

  int id = gettid();

  for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
    ThreadInfo* info = sRegisteredThreads->at(i);
    if (info->ThreadId() == id && !info->IsPendingDelete()) {
      if (profiler_is_active()) {
        
        
        
        info->SetPendingDelete();
        break;
      } else {
        delete info;
        sRegisteredThreads->erase(sRegisteredThreads->begin() + i);
        break;
      }
    }
  }
}

void TickSample::PopulateContext(void* aContext)
{
  
#if defined(SPS_PLAT_amd64_darwin)
  asm (
      
      
      "leaq 0x10(%%rbp), %0\n\t"
      
      "movq (%%rbp), %1\n\t"
      :
      "=r"(sp),
      "=r"(fp)
  );
#elif defined(SPS_PLAT_x86_darwin)
  asm (
      
      
      
      "leal 0xc(%%ebp), %0\n\t"
      
      "movl (%%ebp), %1\n\t"
      :
      "=r"(sp),
      "=r"(fp)
  );
#else
# error "Unsupported architecture"
#endif
  pc = reinterpret_cast<Address>(__builtin_extract_return_addr(
                                    __builtin_return_address(0)));
}

