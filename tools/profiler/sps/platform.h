



#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif

#include "mozilla/Util.h"
#include "mozilla/unused.h"
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

  
  virtual void SampleStack(TickSample* sample) = 0;

  
  
  virtual void Tick(TickSample* sample) = 0;

  
  virtual void RequestSave() = 0;
  
  virtual void HandleSaveRequest() = 0;

  
  void Start();
  void Stop();

  
  bool IsProfiling() const { return profiling_; }

  
  
  
  
  bool IsSynchronous() const { return synchronous_; }

  
  bool IsActive() const { return active_; }

  class PlatformData;

 private:
  const int interval_;
  const bool profiling_;
  const bool synchronous_;
  bool active_;
  PlatformData* data_;  
};

