



























#ifndef TOOLS_PLATFORM_H_
#define TOOLS_PLATFORM_H_

#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif

#ifdef XP_UNIX
#include <pthread.h>
#endif

#include <stdint.h>
#include <math.h>
#include "MainThreadUtils.h"
#include "mozilla/unused.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Mutex.h"
#include "PlatformMacros.h"
#include "ThreadResponsiveness.h"
#include "v8-support.h"
#include <vector>

#ifdef XP_WIN
#include <windows.h>
#endif

#define ASSERT(a) MOZ_ASSERT(a)

bool moz_profiler_verbose();

#ifdef ANDROID
# if defined(__arm__) || defined(__thumb__)
#  define ENABLE_SPS_LEAF_DATA
#  define ENABLE_ARM_LR_SAVING
# endif
# define LOG(text) \
    do { if (moz_profiler_verbose()) \
           __android_log_write(ANDROID_LOG_ERROR, "Profiler", text); \
    } while (0)
# define LOGF(format, ...) \
    do { if (moz_profiler_verbose()) \
           __android_log_print(ANDROID_LOG_ERROR, "Profiler", format, \
                               __VA_ARGS__); \
    } while (0)

#else
# define LOG(text) \
    do { if (moz_profiler_verbose()) fprintf(stderr, "Profiler: %s\n", text); \
    } while (0)
# define LOGF(format, ...) \
    do { if (moz_profiler_verbose()) fprintf(stderr, "Profiler: " format \
                                             "\n", __VA_ARGS__);        \
    } while (0)

#endif

#if defined(XP_MACOSX) || defined(XP_WIN) || defined(XP_LINUX)
#define ENABLE_SPS_LEAF_DATA
#endif

extern mozilla::TimeStamp sStartTime;

typedef uint8_t* Address;







class Mutex {
 public:
  virtual ~Mutex() {}

  
  
  
  
  virtual int Lock() = 0;

  
  
  virtual int Unlock() = 0;

  
  
  virtual bool TryLock() = 0;
};








class OS {
 public:

  
  static void Sleep(const int milliseconds);

  
  static void SleepMicro(const int microseconds);

  
  static void Startup();

 private:
  static const int msPerSecond = 1000;

};












class Thread {
 public:
  
  explicit Thread(const char* name);
  virtual ~Thread();

  
  void Start();

  void Join();

  inline const char* name() const {
    return name_;
  }

  
  virtual void Run() = 0;

  
  
  static const int kMaxThreadNameLength = 16;

#ifdef XP_WIN
  HANDLE thread_;
  typedef DWORD tid_t;
  tid_t thread_id_;
#else
  typedef ::pid_t tid_t;
#endif
#if defined(XP_MACOSX)
  pthread_t thread_;
#endif

  static tid_t GetCurrentId();

 private:
  void set_name(const char *name);

  char name_[kMaxThreadNameLength];
  int stack_size_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};










#undef HAVE_NATIVE_UNWIND
#if defined(MOZ_PROFILING) \
    && (defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
        || defined(SPS_PLAT_x86_linux) \
        || defined(SPS_OS_windows) \
        || defined(SPS_OS_darwin))
# define HAVE_NATIVE_UNWIND
#endif



extern const char* PROFILER_INTERVAL;
extern const char* PROFILER_ENTRIES;
extern const char* PROFILER_STACK;
extern const char* PROFILER_FEATURES;

void read_profiler_env_vars();
void profiler_usage();


bool set_profiler_interval(const char*);
bool set_profiler_entries(const char*);
bool set_profiler_scan(const char*);
bool is_native_unwinding_avail();

void set_tls_stack_top(void* stackTop);








struct PseudoStack;
class ThreadProfile;


class TickSample {
 public:
  TickSample()
      :
        pc(NULL),
        sp(NULL),
        fp(NULL),
#ifdef ENABLE_ARM_LR_SAVING
        lr(NULL),
#endif
        context(NULL),
        isSamplingCurrentThread(false),
        threadProfile(nullptr),
        rssMemory(0),
        ussMemory(0) {}

  void PopulateContext(void* aContext);

  Address pc;  
  Address sp;  
  Address fp;  
#ifdef ENABLE_ARM_LR_SAVING
  Address lr;  
#endif
  void*   context;   
                     
  bool    isSamplingCurrentThread;
  ThreadProfile* threadProfile;
  mozilla::TimeStamp timestamp;
  int64_t rssMemory;
  int64_t ussMemory;
};

class ThreadInfo;
class PlatformData;
class TableTicker;
class SyncProfile;
class Sampler {
 public:
  
  explicit Sampler(double interval, bool profiling, int entrySize);
  virtual ~Sampler();

  double interval() const { return interval_; }

  
  
  virtual void Tick(TickSample* sample) = 0;

  
  virtual SyncProfile* GetBacktrace() = 0;

  
  virtual void RequestSave() = 0;
  
  virtual void HandleSaveRequest() = 0;
  
  virtual void DeleteExpiredMarkers() = 0;

  
  void Start();
  void Stop();

  
  bool IsProfiling() const { return profiling_; }

  
  bool IsActive() const { return active_; }

  
  bool IsPaused() const { return paused_; }
  void SetPaused(bool value) { NoBarrier_Store(&paused_, value); }

  virtual bool ProfileThreads() const = 0;

  int EntrySize() { return entrySize_; }

  
  
  static PlatformData* AllocPlatformData(int aThreadId);
  static void FreePlatformData(PlatformData*);

  
  
#ifdef XP_WIN
  
  static uintptr_t GetThreadHandle(PlatformData*);
#endif
#ifdef XP_MACOSX
  static pthread_t GetProfiledThread(PlatformData*);
#endif

  static std::vector<ThreadInfo*> GetRegisteredThreads() {
    return *sRegisteredThreads;
  }

  static bool RegisterCurrentThread(const char* aName,
                                    PseudoStack* aPseudoStack,
                                    bool aIsMainThread, void* stackTop);
  static void UnregisterCurrentThread();

  static void Startup();
  
  static void Shutdown();

  static TableTicker* GetActiveSampler() { return sActiveSampler; }
  static void SetActiveSampler(TableTicker* sampler) { sActiveSampler = sampler; }

  static mozilla::Mutex* sRegisteredThreadsMutex;

  static bool CanNotifyObservers() {
#ifdef MOZ_WIDGET_GONK
    
    return false;
#elif defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
    
    return NS_IsMainThread();
#else
    MOZ_ASSERT(NS_IsMainThread());
    return true;
#endif
  }

 protected:
  static std::vector<ThreadInfo*>* sRegisteredThreads;
  static TableTicker* sActiveSampler;

 private:
  void SetActive(bool value) { NoBarrier_Store(&active_, value); }

  const double interval_;
  const bool profiling_;
  Atomic32 paused_;
  Atomic32 active_;
  const int entrySize_;

  
#if defined(SPS_OS_linux) || defined(SPS_OS_android)
  bool signal_handler_installed_;
  struct sigaction old_sigprof_signal_handler_;
  struct sigaction old_sigsave_signal_handler_;
  bool signal_sender_launched_;
  pthread_t signal_sender_thread_;
#endif
#if defined(SPS_OS_darwin)
  bool signal_handler_installed_;
  struct sigaction old_sigprof_signal_handler_;
#endif
};

class ThreadInfo {
 public:
  ThreadInfo(const char* aName, int aThreadId, bool aIsMainThread, PseudoStack* aPseudoStack, void* aStackTop);

  virtual ~ThreadInfo();

  const char* Name() const { return mName; }
  int ThreadId() const { return mThreadId; }

  bool IsMainThread() const { return mIsMainThread; }
  PseudoStack* Stack() const { return mPseudoStack; }

  void SetProfile(ThreadProfile* aProfile) { mProfile = aProfile; }
  ThreadProfile* Profile() const { return mProfile; }

  PlatformData* GetPlatformData() const { return mPlatformData; }
  void* StackTop() const { return mStackTop; }

  virtual void SetPendingDelete();
  bool IsPendingDelete() const { return mPendingDelete; }

#ifdef MOZ_NUWA_PROCESS
  void SetThreadId(int aThreadId) { mThreadId = aThreadId; }
#endif

  


  nsIThread* GetThread() const { return mThread.get(); }
 private:
  char* mName;
  int mThreadId;
  const bool mIsMainThread;
  PseudoStack* mPseudoStack;
  PlatformData* mPlatformData;
  ThreadProfile* mProfile;
  void* const mStackTop;
  nsCOMPtr<nsIThread> mThread;
  bool mPendingDelete;
};


class StackOwningThreadInfo : public ThreadInfo {
 public:
  StackOwningThreadInfo(const char* aName, int aThreadId, bool aIsMainThread, PseudoStack* aPseudoStack, void* aStackTop);
  virtual ~StackOwningThreadInfo();

  virtual void SetPendingDelete();
};

#endif 
