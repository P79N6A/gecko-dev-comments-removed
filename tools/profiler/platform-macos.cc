



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


#include "platform.h"





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



static const pthread_t kNoThread = (pthread_t) 0;

class MacOSMutex : public Mutex {
 public:
  MacOSMutex() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_, &attr);
  }

  virtual ~MacOSMutex() { pthread_mutex_destroy(&mutex_); }

  virtual int Lock() { return pthread_mutex_lock(&mutex_); }
  virtual int Unlock() { return pthread_mutex_unlock(&mutex_); }

  virtual bool TryLock() {
    int result = pthread_mutex_trylock(&mutex_);
    
    if (result == EBUSY) {
      return false;
    }
    ASSERT(result == 0);  
    return true;
  }

 private:
  pthread_mutex_t mutex_;
};


Mutex* OS::CreateMutex() {
  return new MacOSMutex();
}

void OS::Sleep(int milliseconds) {
  usleep(1000 * milliseconds);
}

class Thread::PlatformData : public Malloced {
 public:
  PlatformData() : thread_(kNoThread) {}
  pthread_t thread_;  
};

Thread::Thread(const char* name)
    : data_(new PlatformData),
      stack_size_(0) {
  set_name(name);
}


Thread::~Thread() {
  delete data_;
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
  
  
  
  thread->data()->thread_ = pthread_self();
  SetThreadName(thread->name());
  ASSERT(thread->data()->thread_ != kNoThread);
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
  pthread_create(&data_->thread_, attr_ptr, ThreadEntry, this);
  ASSERT(data_->thread_ != kNoThread);
}

void Thread::Join() {
  pthread_join(data_->thread_, NULL);
}

class Sampler::PlatformData : public Malloced {
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


class SamplerThread : public Thread {
 public:
  explicit SamplerThread(int interval)
      : Thread("SamplerThread"),
        interval_(interval) {}

  static void AddActiveSampler(Sampler* sampler) {
    ScopedLock lock(mutex_);
    SamplerRegistry::AddActiveSampler(sampler);
    if (instance_ == NULL) {
      instance_ = new SamplerThread(sampler->interval());
      instance_->Start();
    } else {
      ASSERT(instance_->interval_ == sampler->interval());
    }
  }

  static void RemoveActiveSampler(Sampler* sampler) {
    ScopedLock lock(mutex_);
    instance_->Join();
    
    
    SamplerRegistry::RemoveActiveSampler(sampler);
    delete instance_;
    instance_ = NULL;
    






  }

  
  virtual void Run() {
    while (SamplerRegistry::sampler->IsActive()) {
      if (!SamplerRegistry::sampler->IsPaused())
        SampleContext(SamplerRegistry::sampler);
      OS::Sleep(interval_);
    }
  }

  void SampleContext(Sampler* sampler) {
    thread_act_t profiled_thread = sampler->platform_data()->profiled_thread();
    TickSample sample_obj;
    TickSample* sample = &sample_obj;
    
    

    if (KERN_SUCCESS != thread_suspend(profiled_thread)) return;

#if V8_HOST_ARCH_X64
    thread_state_flavor_t flavor = x86_THREAD_STATE64;
    x86_thread_state64_t state;
    mach_msg_type_number_t count = x86_THREAD_STATE64_COUNT;
#if __DARWIN_UNIX03
#define REGISTER_FIELD(name) __r ## name
#else
#define REGISTER_FIELD(name) r ## name
#endif  
#elif V8_HOST_ARCH_IA32
    thread_state_flavor_t flavor = i386_THREAD_STATE;
    i386_thread_state_t state;
    mach_msg_type_number_t count = i386_THREAD_STATE_COUNT;
#if __DARWIN_UNIX03
#define REGISTER_FIELD(name) __e ## name
#else
#define REGISTER_FIELD(name) e ## name
#endif  
#else
#error Unsupported Mac OS X host architecture.
#endif  

    if (thread_get_state(profiled_thread,
                         flavor,
                         reinterpret_cast<natural_t*>(&state),
                         &count) == KERN_SUCCESS) {
      
      sample->pc = reinterpret_cast<Address>(state.REGISTER_FIELD(ip));
      sample->sp = reinterpret_cast<Address>(state.REGISTER_FIELD(sp));
      sample->fp = reinterpret_cast<Address>(state.REGISTER_FIELD(bp));
      sample->timestamp = mozilla::TimeStamp::Now();
      sampler->SampleStack(sample);
      sampler->Tick(sample);
    }
    thread_resume(profiled_thread);
  }

  const int interval_;
  

  
  static Mutex* mutex_;
  static SamplerThread* instance_;

  DISALLOW_COPY_AND_ASSIGN(SamplerThread);
};

#undef REGISTER_FIELD


Mutex* SamplerThread::mutex_ = OS::CreateMutex();
SamplerThread* SamplerThread::instance_ = NULL;


Sampler::Sampler(int interval, bool profiling)
    : 
      interval_(interval),
      profiling_(profiling),
      paused_(false),
      active_(false) 
 {
  data_ = new PlatformData;
}


Sampler::~Sampler() {
  ASSERT(!IsActive());
  delete data_;
}


void Sampler::Start() {
  ASSERT(!IsActive());
  SetActive(true);
  SamplerThread::AddActiveSampler(this);
}


void Sampler::Stop() {
  ASSERT(IsActive());
  SetActive(false);
  SamplerThread::RemoveActiveSampler(this);
}

pthread_t
Sampler::GetProfiledThread(Sampler::PlatformData* aData)
{
  return aData->profiled_pthread();
}
