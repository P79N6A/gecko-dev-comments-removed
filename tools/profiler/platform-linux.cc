






























#include <stdio.h>
#include <math.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/prctl.h> 
#include <stdlib.h>
#include <sched.h>
#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif
#include <ucontext.h>



#include <sys/types.h>  
#include <sys/mman.h>   
#include <sys/stat.h>   
#include <fcntl.h>      
#include <unistd.h>     
#include <semaphore.h>
#ifdef __GLIBC__
#include <execinfo.h>   
#endif  
#include <strings.h>    
#include <errno.h>
#include <stdarg.h>
#include "prenv.h"
#include "platform.h"
#include "GeckoProfiler.h"
#include "mozilla/Mutex.h"
#include "mozilla/Atomics.h"
#include "mozilla/LinuxSignal.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/DebugOnly.h"
#include "ProfileEntry.h"
#include "nsThreadUtils.h"
#include "TableTicker.h"
#include "ThreadResponsiveness.h"

#if defined(__ARM_EABI__) && defined(MOZ_WIDGET_GONK)
 
# define USE_EHABI_STACKWALK
# include "EHABIStackWalk.h"
#elif defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_x86_linux)
# define USE_LUL_STACKWALK
# include "LulMain.h"
# include "platform-linux-lul.h"
#endif


#include "nsMemoryReporterManager.h"

#include <string.h>
#include <list>

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

#define SIGNAL_SAVE_PROFILE SIGUSR2

using namespace mozilla;

#if defined(USE_LUL_STACKWALK)




lul::LUL* sLUL = nullptr;


static void sLUL_initialization_routine(void)
{
  MOZ_ASSERT(!sLUL);
  MOZ_ASSERT(gettid() == getpid()); 
  sLUL = new lul::LUL(logging_sink_for_LUL);
  
  read_procmaps(sLUL);
}
#endif

 Thread::tid_t
Thread::GetCurrentId()
{
  return gettid();
}

#if !defined(ANDROID)












static bool was_paused = false;



static void paf_prepare(void) {
  if (Sampler::GetActiveSampler()) {
    was_paused = Sampler::GetActiveSampler()->IsPaused();
    Sampler::GetActiveSampler()->SetPaused(true);
  } else {
    was_paused = false;
  }
}



static void paf_parent(void) {
  if (Sampler::GetActiveSampler())
    Sampler::GetActiveSampler()->SetPaused(was_paused);
}


static void* setup_atfork() {
  pthread_atfork(paf_prepare, paf_parent, NULL);
  return NULL;
}
#endif 

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
static sem_t sSignalHandlingDone;

static void ProfilerSaveSignalHandler(int signal, siginfo_t* info, void* context) {
  Sampler::GetActiveSampler()->RequestSave();
}

static void SetSampleContext(TickSample* sample, void* context)
{
  
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

#ifdef ANDROID
#define V8_HOST_ARCH_ARM 1
#define SYS_gettid __NR_gettid
#define SYS_tgkill __NR_tgkill
#else
#define V8_HOST_ARCH_X64 1
#endif

namespace {

void ProfilerSignalHandler(int signal, siginfo_t* info, void* context) {
  if (!Sampler::GetActiveSampler()) {
    sem_post(&sSignalHandlingDone);
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
  sem_post(&sSignalHandlingDone);
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

int tgkill(pid_t tgid, pid_t tid, int signalno) {
  return syscall(SYS_tgkill, tgid, tid, signalno);
}

class PlatformData : public Malloced {
 public:
  PlatformData()
  {}
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

static void* SignalSender(void* arg) {
  
  prctl(PR_SET_NAME, "SamplerThread", 0, 0, 0);

#ifdef MOZ_NUWA_PROCESS
  
  
  
  if(IsNuwaProcess()) {
    NuwaMarkCurrentThread(nullptr, nullptr);
    
    
    NuwaFreezeCurrentThread();
  }
#endif

  int vm_tgid_ = getpid();
  DebugOnly<int> my_tid = gettid();

  unsigned int nSignalsSent = 0;

  TimeDuration lastSleepOverhead = 0;
  TimeStamp sampleStart = TimeStamp::Now();
  while (SamplerRegistry::sampler->IsActive()) {

    SamplerRegistry::sampler->HandleSaveRequest();
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

        
        
        sCurrentThreadProfile = info->Profile();

        int threadId = info->ThreadId();
        MOZ_ASSERT(threadId != my_tid);

        
        
        
        ProfilerSignalThread(sCurrentThreadProfile, isFirstProfiledThread);

        
        
        
        if (tgkill(vm_tgid_, threadId, SIGPROF) != 0) {
          printf_stderr("profiler failed to signal tid=%d\n", threadId);
#ifdef DEBUG
          abort();
#endif
          continue;
        }

        
        sem_wait(&sSignalHandlingDone);
        isFirstProfiledThread = false;

        
        
        
        
        
        if ((++nSignalsSent & 0xF) == 0) {
#          if defined(USE_LUL_STACKWALK)
           sLUL->MaybeShowStats();
#          endif
        }
      }
    }

    TimeStamp targetSleepEndTime = sampleStart + TimeDuration::FromMicroseconds(SamplerRegistry::sampler->interval() * 1000);
    TimeStamp beforeSleep = TimeStamp::Now();
    TimeDuration targetSleepDuration = targetSleepEndTime - beforeSleep;
    double sleepTime = std::max(0.0, (targetSleepDuration - lastSleepOverhead).ToMicroseconds());
    OS::SleepMicro(sleepTime);
    sampleStart = TimeStamp::Now();
    lastSleepOverhead = sampleStart - (beforeSleep + TimeDuration::FromMicroseconds(sleepTime));
  }
  return 0;
}

Sampler::Sampler(double interval, bool profiling, int entrySize)
    : interval_(interval),
      profiling_(profiling),
      paused_(false),
      active_(false),
      entrySize_(entrySize) {
}

Sampler::~Sampler() {
  ASSERT(!signal_sender_launched_);
}


void Sampler::Start() {
  LOG("Sampler started");

#if defined(USE_EHABI_STACKWALK)
  mozilla::EHABIStackWalkInit();
#elif defined(USE_LUL_STACKWALK)
  
  
  if (!sLUL) {
     sLUL_initialization_routine();
  }
#endif

  SamplerRegistry::AddActiveSampler(this);

  
  sCurrentThreadProfile = NULL;
  if (sem_init(&sSignalHandlingDone,  0,  0) != 0) {
    LOG("Error initializing semaphore");
    return;
  }

  
  LOG("Request signal");
  struct sigaction sa;
  sa.sa_sigaction = MOZ_SIGNAL_TRAMPOLINE(ProfilerSignalHandler);
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGPROF, &sa, &old_sigprof_signal_handler_) != 0) {
    LOG("Error installing signal");
    return;
  }

  
  struct sigaction sa2;
  sa2.sa_sigaction = ProfilerSaveSignalHandler;
  sigemptyset(&sa2.sa_mask);
  sa2.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGNAL_SAVE_PROFILE, &sa2, &old_sigsave_signal_handler_) != 0) {
    LOG("Error installing start signal");
    return;
  }
  LOG("Signal installed");
  signal_handler_installed_ = true;

#if defined(USE_LUL_STACKWALK)
  
  
  
  sLUL->EnableUnwinding();

  
  if (PR_GetEnv("MOZ_PROFILER_LUL_TEST")) {
     int nTests = 0, nTestsPassed = 0;
     RunLulUnitTests(&nTests, &nTestsPassed, sLUL);
  }
#endif

  
  
  
  SetActive(true);
  if (pthread_create(
        &signal_sender_thread_, NULL, SignalSender, NULL) == 0) {
    signal_sender_launched_ = true;
  }
  LOG("Profiler thread started");
}


void Sampler::Stop() {
  SetActive(false);

  
  
  if (signal_sender_launched_) {
    pthread_join(signal_sender_thread_, NULL);
    signal_sender_launched_ = false;
  }

  SamplerRegistry::RemoveActiveSampler(this);

  
  if (signal_handler_installed_) {
    sigaction(SIGNAL_SAVE_PROFILE, &old_sigsave_signal_handler_, 0);
    sigaction(SIGPROF, &old_sigprof_signal_handler_, 0);
    signal_handler_installed_ = false;
  }
}

#ifdef MOZ_NUWA_PROCESS
static void
UpdateThreadId(void* aThreadInfo) {
  ThreadInfo* info = static_cast<ThreadInfo*>(aThreadInfo);
  
  
  
  
  info->SetThreadId(gettid());
}
#endif

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

#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaProcess()) {
    if (info->IsMainThread()) {
      
      
      
      NuwaAddConstructor(UpdateThreadId, info);
    } else {
      
      NuwaAddThreadConstructor(UpdateThreadId, info);
    }
  }
#endif

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

#ifdef ANDROID
static struct sigaction old_sigstart_signal_handler;
const int SIGSTART = SIGUSR2;

static void freeArray(const char** array, int size) {
  for (int i = 0; i < size; i++) {
    free((void*) array[i]);
  }
}

static uint32_t readCSVArray(char* csvList, const char** buffer) {
  uint32_t count;
  char* savePtr;
  int newlinePos = strlen(csvList) - 1;
  if (csvList[newlinePos] == '\n') {
    csvList[newlinePos] = '\0';
  }

  char* item = strtok_r(csvList, ",", &savePtr);
  for (count = 0; item; item = strtok_r(NULL, ",", &savePtr)) {
    int length = strlen(item) + 1;  
    char* newBuf = (char*) malloc(sizeof(char) * length);
    buffer[count] = newBuf;
    strncpy(newBuf, item, length);
    count++;
  }

  return count;
}



static void ReadProfilerVars(const char* fileName, const char** features,
                            uint32_t* featureCount, const char** threadNames, uint32_t* threadCount) {
  FILE* file = fopen(fileName, "r");
  const int bufferSize = 1024;
  char line[bufferSize];
  char* feature;
  char* value;
  char* savePtr;

  if (file) {
    while (fgets(line, bufferSize, file) != NULL) {
      feature = strtok_r(line, "=", &savePtr);
      value = strtok_r(NULL, "", &savePtr);

      if (strncmp(feature, PROFILER_INTERVAL, bufferSize) == 0) {
        set_profiler_interval(value);
      } else if (strncmp(feature, PROFILER_ENTRIES, bufferSize) == 0) {
        set_profiler_entries(value);
      } else if (strncmp(feature, PROFILER_STACK, bufferSize) == 0) {
        set_profiler_scan(value);
      } else if (strncmp(feature, PROFILER_FEATURES, bufferSize) == 0) {
        *featureCount = readCSVArray(value, features);
      } else if (strncmp(feature, "threads", bufferSize) == 0) {
        *threadCount = readCSVArray(value, threadNames);
      }
    }

    fclose(file);
  }
}


static void StartSignalHandler(int signal, siginfo_t* info, void* context) {
  
  
  
  uint32_t featureCount = 0;
  uint32_t threadCount = 0;

  
  
  
  
  const char* threadNames[10];
  const char* features[10];
  const char* profilerConfigFile = "/data/local/tmp/profiler.options";

  ReadProfilerVars(profilerConfigFile, features, &featureCount, threadNames, &threadCount);
  MOZ_ASSERT(featureCount < 10);
  MOZ_ASSERT(threadCount < 10);

  profiler_start(PROFILE_DEFAULT_ENTRY, 1,
      features, featureCount,
      threadNames, threadCount);

  freeArray(threadNames, threadCount);
  freeArray(features, featureCount);
}

void OS::Startup()
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

#else

void OS::Startup() {
  
  setup_atfork();
}

#endif



void TickSample::PopulateContext(void* aContext)
{
  MOZ_ASSERT(aContext);
  ucontext_t* pContext = reinterpret_cast<ucontext_t*>(aContext);
  if (!getcontext(pContext)) {
    context = pContext;
    SetSampleContext(this, aContext);
  }
}

void OS::SleepMicro(int microseconds)
{
  if (MOZ_UNLIKELY(microseconds >= 1000000)) {
    
    
    MOZ_ALWAYS_TRUE(!::usleep(microseconds));
    return;
  }

  struct timespec ts;
  ts.tv_sec  = 0;
  ts.tv_nsec = microseconds * 1000UL;

  int rv = ::nanosleep(&ts, &ts);

  while (rv != 0 && errno == EINTR) {
    
    
    rv = ::nanosleep(&ts, &ts);
  }

  MOZ_ASSERT(!rv, "nanosleep call failed");
}
