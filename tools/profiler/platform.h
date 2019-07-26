



























#ifndef TOOLS_PLATFORM_H_
#define TOOLS_PLATFORM_H_




#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif

#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"
#include "mozilla/unused.h"
#include "mozilla/TimeStamp.h"
#include "PlatformMacros.h"
#include "mozilla/Mutex.h"
#include "v8-support.h"
#include <vector>
#define ASSERT(a) MOZ_ASSERT(a)
#ifdef ANDROID
#if defined(__arm__) || defined(__thumb__)
#define ENABLE_SPS_LEAF_DATA
#define ENABLE_ARM_LR_SAVING
#endif
#define LOG(text) __android_log_write(ANDROID_LOG_ERROR, "Profiler", text)
#define LOGF(format, ...) __android_log_print(ANDROID_LOG_ERROR, "Profiler", format, __VA_ARGS__)
#elif defined(SPS_FORCE_LOG)
#define LOG(text) fprintf(stderr, "Profiler: %s\n", text)
#define LOGF(format, ...) fprintf(stderr, "Profiler: " format "\n", __VA_ARGS__)
#else
#define LOG(TEXT) do {} while(0)
#define LOGF(format, ...) do {} while(0)
#endif

#if defined(XP_MACOSX) || defined(XP_WIN)
#define ENABLE_SPS_LEAF_DATA
#endif

typedef uint8_t* Address;







class Mutex {
 public:
  virtual ~Mutex() {}

  
  
  
  
  virtual int Lock() = 0;

  
  
  virtual int Unlock() = 0;

  
  
  virtual bool TryLock() = 0;
};






class ScopedLock {
 public:
  explicit ScopedLock(Mutex* mutex): mutex_(mutex) {
    ASSERT(mutex_ != NULL);
    mutex_->Lock();
  }
  ~ScopedLock() {
    mutex_->Unlock();
  }

 private:
  Mutex* mutex_;
  DISALLOW_COPY_AND_ASSIGN(ScopedLock);
};










class OS {
 public:

  
  static void Sleep(const int milliseconds);

  
  
  static Mutex* CreateMutex();

  
  
#if defined(ANDROID)
  static void RegisterStartHandler();
#else
  static void RegisterStartHandler() {}
#endif

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

  class PlatformData;
  PlatformData* data() { return data_; }

 private:
  void set_name(const char *name);

  PlatformData* data_;

  char name_[kMaxThreadNameLength];
  int stack_size_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};











#undef HAVE_NATIVE_UNWIND
#if defined(MOZ_PROFILING) \
    && (defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
        || defined(SPS_PLAT_x86_linux))
# define HAVE_NATIVE_UNWIND
#endif








class PseudoStack;
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
        function(NULL),
        context(NULL),
        frames_count(0) {}
  Address pc;  
  Address sp;  
  Address fp;  
#ifdef ENABLE_ARM_LR_SAVING
  Address lr;  
#endif
  Address function;  
  void*   context;   
                     
  ThreadProfile* threadProfile;
  static const int kMaxFramesCount = 64;
  int frames_count;  
  mozilla::TimeStamp timestamp;
};

class ThreadInfo {
 public:
  ThreadInfo(const char* aName, int aThreadId, bool aIsMainThread, PseudoStack* aPseudoStack)
    : mName(strdup(aName))
    , mThreadId(aThreadId)
    , mIsMainThread(aIsMainThread)
    , mPseudoStack(aPseudoStack)
    , mProfile(NULL) {}

  virtual ~ThreadInfo();

  const char* Name() const { return mName; }
  int ThreadId() const { return mThreadId; }

  bool IsMainThread() const { return mIsMainThread; }
  PseudoStack* Stack() const { return mPseudoStack; }
  
  void SetProfile(ThreadProfile* aProfile) { mProfile = aProfile; }
  ThreadProfile* Profile() const { return mProfile; }

 private:
  char* mName;
  int mThreadId;
  const bool mIsMainThread;
  PseudoStack* mPseudoStack;
  ThreadProfile* mProfile;
};

class Sampler {
 public:
  
  explicit Sampler(int interval, bool profiling, int entrySize);
  virtual ~Sampler();

  int interval() const { return interval_; }

  
  virtual void SampleStack(TickSample* sample) = 0;

  
  
  virtual void Tick(TickSample* sample) = 0;

  
  virtual void RequestSave() = 0;
  
  virtual void HandleSaveRequest() = 0;

  
  void Start();
  void Stop();

  
  bool IsProfiling() const { return profiling_; }

  
  bool IsActive() const { return active_; }

  
  bool IsPaused() const { return paused_; }
  void SetPaused(bool value) { NoBarrier_Store(&paused_, value); }

  int EntrySize() { return entrySize_; }

  class PlatformData;

  PlatformData* platform_data() { return data_; }

  
  
#ifdef XP_WIN
  
  static uintptr_t GetThreadHandle(PlatformData*);
#endif
#ifdef XP_MACOSX
  static pthread_t GetProfiledThread(PlatformData*);
#endif

  static std::vector<ThreadInfo*> GetRegisteredThreads() {
    mozilla::MutexAutoLock lock(sRegisteredThreadsMutex);

    return sRegisteredThreads;
  }

  static bool RegisterCurrentThread(const char* aName, PseudoStack* aPseudoStack, bool aIsMainThread);
  static void UnregisterCurrentThread();

  
  static void ClearRegisteredThreads() {
    while (sRegisteredThreads.size() > 0) {
      sRegisteredThreads.pop_back();
    }
  }

  static Sampler* GetActiveSampler() { return sActiveSampler; }
  static void SetActiveSampler(Sampler* sampler) { sActiveSampler = sampler; }

 protected:
  static std::vector<ThreadInfo*> sRegisteredThreads;
  static mozilla::Mutex sRegisteredThreadsMutex;
  static Sampler* sActiveSampler;

 private:
  void SetActive(bool value) { NoBarrier_Store(&active_, value); }

  const int interval_;
  const bool profiling_;
  Atomic32 paused_;
  Atomic32 active_;
  const int entrySize_;
  PlatformData* data_;  
};

#endif 
