






























#include <stdio.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sched.h>
#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif



#include <sys/types.h>  
#include <sys/mman.h>   
#include <sys/stat.h>   
#include <fcntl.h>      
#include <unistd.h>     
#ifdef __GLIBC__
#include <execinfo.h>   
#endif  
#include <strings.h>    
#include <errno.h>
#include <stdarg.h>
#include "platform.h"
#include "GeckoProfilerImpl.h"
#include "mozilla/Mutex.h"
#include "ProfileEntry.h"
#include "nsThreadUtils.h"

#include <string.h>
#include <stdio.h>
#include <list>

#define SIGNAL_SAVE_PROFILE SIGUSR2

#if defined(__GLIBC__)

#include <sys/syscall.h>
pid_t gettid()
{
  return (pid_t) syscall(SYS_gettid);
}
#endif

#ifdef ANDROID
#include "android-signal-defs.h"
#endif

static ThreadProfile* sCurrentThreadProfile = NULL;

static void ProfilerSaveSignalHandler(int signal, siginfo_t* info, void* context) {
  Sampler::GetActiveSampler()->RequestSave();
}

#ifdef ANDROID
#define V8_HOST_ARCH_ARM 1
#define SYS_gettid __NR_gettid
#define SYS_tgkill __NR_tgkill
#else
#define V8_HOST_ARCH_X64 1
#endif
static void ProfilerSignalHandler(int signal, siginfo_t* info, void* context) {
  if (!Sampler::GetActiveSampler())
    return;

  TickSample sample_obj;
  TickSample* sample = &sample_obj;
  sample->context = context;

#ifdef ENABLE_SPS_LEAF_DATA
  
  if (Sampler::GetActiveSampler()->IsProfiling()) {
    
    ucontext_t* ucontext = reinterpret_cast<ucontext_t*>(context);
    mcontext_t& mcontext = ucontext->uc_mcontext;
#if V8_HOST_ARCH_IA32
    sample->pc = reinterpret_cast<Address>(mcontext.gregs[REG_EIP]);
    sample->sp = reinterpret_cast<Address>(mcontext.gregs[REG_ESP]);
    sample->fp = reinterpret_cast<Address>(mcontext.gregs[REG_EBP]);
#elif V8_HOST_ARCH_X64
    sample->pc = reinterpret_cast<Address>(mcontext.gregs[REG_RIP]);
    sample->sp = reinterpret_cast<Address>(mcontext.gregs[REG_RSP]);
    sample->fp = reinterpret_cast<Address>(mcontext.gregs[REG_RBP]);
#elif V8_HOST_ARCH_ARM

#if !defined(ANDROID) && (__GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ <= 3))
    sample->pc = reinterpret_cast<Address>(mcontext.gregs[R15]);
    sample->sp = reinterpret_cast<Address>(mcontext.gregs[R13]);
    sample->fp = reinterpret_cast<Address>(mcontext.gregs[R11]);
#ifdef ENABLE_ARM_LR_SAVING
    sample->lr = reinterpret_cast<Address>(mcontext.gregs[R14]);
#endif
#else
    sample->pc = reinterpret_cast<Address>(mcontext.arm_pc);
    sample->sp = reinterpret_cast<Address>(mcontext.arm_sp);
    sample->fp = reinterpret_cast<Address>(mcontext.arm_fp);
#ifdef ENABLE_ARM_LR_SAVING
    sample->lr = reinterpret_cast<Address>(mcontext.arm_lr);
#endif
#endif
#elif V8_HOST_ARCH_MIPS
    
    UNIMPLEMENTED();
#endif
  }
#endif
  sample->threadProfile = sCurrentThreadProfile;
  sample->timestamp = mozilla::TimeStamp::Now();

  Sampler::GetActiveSampler()->Tick(sample);

  sCurrentThreadProfile = NULL;
}

#ifndef XP_MACOSX
int tgkill(pid_t tgid, pid_t tid, int signalno) {
  return syscall(SYS_tgkill, tgid, tid, signalno);
}
#endif

class Sampler::PlatformData : public Malloced {
 public:
  explicit PlatformData(Sampler* sampler)
      : sampler_(sampler),
        signal_handler_installed_(false),
        vm_tgid_(getpid()),
#ifndef XP_MACOSX
        vm_tid_(gettid()),
#endif
        signal_sender_launched_(false)
#ifdef XP_MACOSX
        , signal_receiver_(pthread_self())
#endif
  {
  }

  void SignalSender() {
    while (sampler_->IsActive()) {
      sampler_->HandleSaveRequest();

      if (!sampler_->IsPaused()) {
#ifdef XP_MACOSX
        pthread_kill(signal_receiver_, SIGPROF);
#else

        std::vector<ThreadInfo*> threads = GetRegisteredThreads();

        for (uint32_t i = 0; i < threads.size(); i++) {
          ThreadInfo* info = threads[i];

          
          
          sCurrentThreadProfile = info->Profile();

          int threadId = info->ThreadId();
          if (threadId == 0) {
            threadId = vm_tid_;
          }

          if (tgkill(vm_tgid_, threadId, SIGPROF) != 0) {
            printf_stderr("profiler failed to signal tid=%d\n", threadId);
#ifdef DEBUG
            abort();
#endif
            continue;
          }

          
          while (sCurrentThreadProfile)
            sched_yield();
        }
#endif
      }

      
      
      
      const useconds_t interval = sampler_->interval_ * 1000 - 100;
      
      usleep(interval);
    }
  }

  Sampler* sampler_;
  bool signal_handler_installed_;
  struct sigaction old_sigprof_signal_handler_;
  struct sigaction old_sigsave_signal_handler_;
  pid_t vm_tgid_;
  pid_t vm_tid_;
  bool signal_sender_launched_;
  pthread_t signal_sender_thread_;
#ifdef XP_MACOSX
  pthread_t signal_receiver_;
#endif
};


static void* SenderEntry(void* arg) {
  Sampler::PlatformData* data =
      reinterpret_cast<Sampler::PlatformData*>(arg);
  data->SignalSender();
  return 0;
}


Sampler::Sampler(int interval, bool profiling, int entrySize)
    : interval_(interval),
      profiling_(profiling),
      paused_(false),
      active_(false),
      entrySize_(entrySize) {
  data_ = new PlatformData(this);
}

Sampler::~Sampler() {
  ASSERT(!data_->signal_sender_launched_);
  delete data_;
}


void Sampler::Start() {
  LOG("Sampler started");

  
  LOG("Request signal");
  struct sigaction sa;
  sa.sa_sigaction = ProfilerSignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGPROF, &sa, &data_->old_sigprof_signal_handler_) != 0) {
    LOG("Error installing signal");
    return;
  }

  
  struct sigaction sa2;
  sa2.sa_sigaction = ProfilerSaveSignalHandler;
  sigemptyset(&sa2.sa_mask);
  sa2.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGNAL_SAVE_PROFILE, &sa2, &data_->old_sigsave_signal_handler_) != 0) {
    LOG("Error installing start signal");
    return;
  }
  LOG("Signal installed");
  data_->signal_handler_installed_ = true;

  
  
  
  SetActive(true);
  if (pthread_create(
          &data_->signal_sender_thread_, NULL, SenderEntry, data_) == 0) {
    data_->signal_sender_launched_ = true;
  }
  LOG("Profiler thread started");
}


void Sampler::Stop() {
  SetActive(false);

  
  
  if (data_->signal_sender_launched_) {
    pthread_join(data_->signal_sender_thread_, NULL);
    data_->signal_sender_launched_ = false;
  }

  
  if (data_->signal_handler_installed_) {
    sigaction(SIGNAL_SAVE_PROFILE, &data_->old_sigsave_signal_handler_, 0);
    sigaction(SIGPROF, &data_->old_sigprof_signal_handler_, 0);
    data_->signal_handler_installed_ = false;
  }
}

bool Sampler::RegisterCurrentThread(const char* aName, PseudoStack* aPseudoStack, bool aIsMainThread)
{
  mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

  ThreadInfo* info = new ThreadInfo(aName, gettid(), aIsMainThread, aPseudoStack);

  if (sActiveSampler) {
    
    info->SetProfile(new ThreadProfile(info->Name(),
                                       sActiveSampler->EntrySize(),
                                       info->Stack(),
                                       info->ThreadId(),
                                       aIsMainThread));
  }

  sRegisteredThreads->push_back(info);
  return true;
}

void Sampler::UnregisterCurrentThread()
{
  mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

  int id = gettid();

  for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
    ThreadInfo* info = sRegisteredThreads->at(i);
    if (info->ThreadId() == id) {
      delete info;
      sRegisteredThreads->erase(sRegisteredThreads->begin() + i);
      break;
    }
  }
}

#ifdef ANDROID
static struct sigaction old_sigstart_signal_handler;
const int SIGSTART = SIGUSR1;

static void StartSignalHandler(int signal, siginfo_t* info, void* context) {
  profiler_start(PROFILE_DEFAULT_ENTRY, PROFILE_DEFAULT_INTERVAL,
                 PROFILE_DEFAULT_FEATURES, PROFILE_DEFAULT_FEATURE_COUNT);
}

void OS::RegisterStartHandler()
{
  LOG("Registering start signal");
  struct sigaction sa;
  sa.sa_sigaction = StartSignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGSTART, &sa, &old_sigstart_signal_handler) != 0) {
    LOG("Error installing signal");
  }
}
#endif

