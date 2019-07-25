



#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif

#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"
#include "mozilla/unused.h"
#include "mozilla/TimeStamp.h"
#include "v8-support.h"
#include <vector>
#define ASSERT(a) MOZ_ASSERT(a)
#ifdef ANDROID
#if defined(__arm__) || defined(__thumb__)
#define ENABLE_SPS_LEAF_DATA
#define ENABLE_ARM_LR_SAVING
#endif
#define LOG(text) __android_log_print(ANDROID_LOG_ERROR, "profiler", "%s", text);
#else
#define LOG(text) printf("Profiler: %s\n", text)
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

  
  
  static void RegisterStartStopHandlers();

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
  static const int kMaxFramesCount = 64;
  Address stack[kMaxFramesCount];  
  int frames_count;  
  mozilla::TimeStamp timestamp;
};

class Sampler {
 public:
  
  explicit Sampler(int interval, bool profiling);
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

  class PlatformData;

  PlatformData* platform_data() { return data_; }

  
  
#ifdef XP_WIN
  
  static uintptr_t GetThreadHandle(PlatformData*);
#endif
#ifdef XP_MACOSX
  static pthread_t GetProfiledThread(PlatformData*);
#endif
 private:
  void SetActive(bool value) { NoBarrier_Store(&active_, value); }

  const int interval_;
  const bool profiling_;
  Atomic32 paused_;
  Atomic32 active_;
  PlatformData* data_;  
};

