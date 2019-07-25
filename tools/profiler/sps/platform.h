



#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif

#include "mozilla/Util.h"
#include "mozilla/unused.h"
#include "v8-support.h"
#include <vector>
#define ASSERT(a) MOZ_ASSERT(a)
#ifdef ANDROID
#define ENABLE_SPS_LEAF_DATA
#define LOG(text) __android_log_print(ANDROID_LOG_ERROR, "profiler", "%s", text);
#else
#define LOG(text) printf("Profiler: %s\n", text)
#endif

#include <stdint.h>
typedef uint8 byte;
typedef byte* Address;

class MapEntry {
public:
  MapEntry(unsigned long aStart, unsigned long aEnd, unsigned long aOffset, char *aName)
    : mStart(aStart)
    , mEnd(aEnd)
    , mOffset(aOffset)
    , mName(strdup(aName))
  {}

  MapEntry(const MapEntry& aEntry)
    : mStart(aEntry.mStart)
    , mEnd(aEntry.mEnd)
    , mOffset(aEntry.mOffset)
    , mName(strdup(aEntry.mName))
  {}

  ~MapEntry()
  {
    free(mName);
  }

  unsigned long GetStart() { return mStart; }
  unsigned long GetEnd() { return mEnd; }
  char* GetName() { return mName; }

private:
  unsigned long mStart;
  unsigned long mEnd;
  unsigned long mOffset;
  char *mName;
};

class MapInfo {
public:
  MapInfo() {}

  void AddMapEntry(MapEntry entry)
  {
    mEntries.push_back(entry);
  }

  MapEntry& GetEntry(size_t i)
  {
    return mEntries[i];
  }

  size_t GetSize()
  {
    return mEntries.size();
  }
private:
  std::vector<MapEntry> mEntries;
};

#ifdef ENABLE_SPS_LEAF_DATA
struct MapInfo getmaps(pid_t pid);
#endif






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

 private:
  static const int msPerSecond = 1000;

};












class Thread {
 public:

  
  explicit Thread(const char* name);
  virtual ~Thread();

  
  void Start();

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
        function(NULL),
        frames_count(0) {}
  Address pc;  
  Address sp;  
  Address fp;  
  Address function;  
  static const int kMaxFramesCount = 64;
  Address stack[kMaxFramesCount];  
  int frames_count;  
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

  class PlatformData;

  PlatformData* platform_data() { return data_; }

 private:
  void SetActive(bool value) { NoBarrier_Store(&active_, value); }

  const int interval_;
  const bool profiling_;
  const bool synchronous_;
  Atomic32 active_;
  PlatformData* data_;  
};

