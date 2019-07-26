




#ifndef PROFILER_PSEUDO_STACK_H_
#define PROFILER_PSEUDO_STACK_H_

#include "mozilla/ArrayUtils.h"
#include "mozilla/NullPtr.h"
#include <stdint.h>
#include "js/ProfilingStack.h"
#include <stdlib.h>
#include "mozilla/Atomics.h"



#if defined(_M_X64) || defined(__x86_64__)
#define V8_HOST_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define V8_HOST_ARCH_IA32 1
#elif defined(__ARMEL__)
#define V8_HOST_ARCH_ARM 1
#else
#warning Please add support for your architecture in chromium_types.h
#endif





#ifdef V8_HOST_ARCH_ARM



typedef void (*LinuxKernelMemoryBarrierFunc)(void);
LinuxKernelMemoryBarrierFunc pLinuxKernelMemoryBarrier __attribute__((weak)) =
    (LinuxKernelMemoryBarrierFunc) 0xffff0fa0;

# define STORE_SEQUENCER() pLinuxKernelMemoryBarrier()
#elif defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_X64)
# if defined(_MSC_VER)
#if _MSC_VER > 1400
#  include <intrin.h>
#else 
    
#ifdef _WINNT_
#  define _interlockedbittestandreset _interlockedbittestandreset_NAME_CHANGED_TO_AVOID_MSVS2005_ERROR
#  define _interlockedbittestandset _interlockedbittestandset_NAME_CHANGED_TO_AVOID_MSVS2005_ERROR
#  include <intrin.h>
#else
#  include <intrin.h>
#  define _interlockedbittestandreset _interlockedbittestandreset_NAME_CHANGED_TO_AVOID_MSVS2005_ERROR
#  define _interlockedbittestandset _interlockedbittestandset_NAME_CHANGED_TO_AVOID_MSVS2005_ERROR
#endif
   
   
#  pragma intrinsic(_ReadWriteBarrier)
#endif 
#  define STORE_SEQUENCER() _ReadWriteBarrier();
# elif defined(__INTEL_COMPILER)
#  define STORE_SEQUENCER() __memory_barrier();
# elif __GNUC__
#  define STORE_SEQUENCER() asm volatile("" ::: "memory");
# else
#  error "Memory clobber not supported for your compiler."
# endif
#else
# error "Memory clobber not supported for your platform."
#endif



static inline uint32_t sMin(uint32_t l, uint32_t r) {
  return l < r ? l : r;
}









class StackEntry : public js::ProfileEntry
{
};

class ProfilerMarkerPayload;
template<typename T>
class ProfilerLinkedList;
class JSStreamWriter;
class JSCustomArray;
class ThreadProfile;
class ProfilerMarker {
  friend class ProfilerLinkedList<ProfilerMarker>;
public:
  ProfilerMarker(const char* aMarkerName,
         ProfilerMarkerPayload* aPayload = nullptr,
         float aTime = 0);

  ~ProfilerMarker();

  const char* GetMarkerName() const {
    return mMarkerName;
  }

  void
  StreamJSObject(JSStreamWriter& b) const;

  void SetGeneration(int aGenID);

  bool HasExpired(int aGenID) const {
    return mGenID + 2 <= aGenID;
  }

  float GetTime();

private:
  char* mMarkerName;
  ProfilerMarkerPayload* mPayload;
  ProfilerMarker* mNext;
  float mTime;
  int mGenID;
};


typedef struct _UnwinderThreadBuffer UnwinderThreadBuffer;






struct LinkedUWTBuffer
{
  LinkedUWTBuffer()
    :mNext(nullptr)
  {}
  virtual ~LinkedUWTBuffer() {}
  virtual UnwinderThreadBuffer* GetBuffer() = 0;
  LinkedUWTBuffer*  mNext;
};

template<typename T>
class ProfilerLinkedList {
public:
  ProfilerLinkedList()
    : mHead(nullptr)
    , mTail(nullptr)
  {}

  void insert(T* elem)
  {
    if (!mTail) {
      mHead = elem;
      mTail = elem;
    } else {
      mTail->mNext = elem;
      mTail = elem;
    }
    elem->mNext = nullptr;
  }

  T* popHead()
  {
    if (!mHead) {
      MOZ_ASSERT(false);
      return nullptr;
    }

    T* head = mHead;

    mHead = head->mNext;
    if (!mHead) {
      mTail = nullptr;
    }

    return head;
  }

  const T* peek() {
    return mHead;
  }

private:
  T* mHead;
  T* mTail;
};

typedef ProfilerLinkedList<ProfilerMarker> ProfilerMarkerLinkedList;
typedef ProfilerLinkedList<LinkedUWTBuffer> UWTBufferLinkedList;

class PendingMarkers {
public:
  PendingMarkers()
    : mSignalLock(false)
  {}

  ~PendingMarkers();

  void addMarker(ProfilerMarker *aMarker);

  void updateGeneration(int aGenID);

  




  void addStoredMarker(ProfilerMarker *aStoredMarker);

  
  ProfilerMarkerLinkedList* getPendingMarkers()
  {
    
    
    
    
    
    if (mSignalLock) {
      return nullptr;
    }
    return &mPendingMarkers;
  }

  void clearMarkers()
  {
    while (mPendingMarkers.peek()) {
      delete mPendingMarkers.popHead();
    }
    while (mStoredMarkers.peek()) {
      delete mStoredMarkers.popHead();
    }
  }

private:
  
  ProfilerMarkerLinkedList mPendingMarkers;
  ProfilerMarkerLinkedList mStoredMarkers;
  
  volatile bool mSignalLock;
  
  
  volatile mozilla::sig_safe_t mGenID;
};

class PendingUWTBuffers
{
public:
  PendingUWTBuffers()
    : mSignalLock(false)
  {
  }

  void addLinkedUWTBuffer(LinkedUWTBuffer* aBuff)
  {
    MOZ_ASSERT(aBuff);
    mSignalLock = true;
    STORE_SEQUENCER();
    mPendingUWTBuffers.insert(aBuff);
    STORE_SEQUENCER();
    mSignalLock = false;
  }

  
  UWTBufferLinkedList* getLinkedUWTBuffers()
  {
    if (mSignalLock) {
      return nullptr;
    }
    return &mPendingUWTBuffers;
  }

private:
  UWTBufferLinkedList mPendingUWTBuffers;
  volatile bool       mSignalLock;
};


void ProfilerJSEventMarker(const char *event);



struct PseudoStack
{
public:
  PseudoStack()
    : mStackPointer(0)
    , mSleepId(0)
    , mSleepIdObserved(0)
    , mSleeping(false)
    , mRuntime(nullptr)
    , mStartJSSampling(false)
    , mPrivacyMode(false)
  { }

  ~PseudoStack() {
    if (mStackPointer != 0) {
      
      
      
      
      abort();
    }
  }

  
  void reinitializeOnResume() {
    
    
    mSleepId++;
  }

  void addLinkedUWTBuffer(LinkedUWTBuffer* aBuff)
  {
    mPendingUWTBuffers.addLinkedUWTBuffer(aBuff);
  }

  UWTBufferLinkedList* getLinkedUWTBuffers()
  {
    return mPendingUWTBuffers.getLinkedUWTBuffers();
  }

  void addMarker(const char *aMarkerStr, ProfilerMarkerPayload *aPayload, float aTime)
  {
    ProfilerMarker* marker = new ProfilerMarker(aMarkerStr, aPayload, aTime);
    mPendingMarkers.addMarker(marker);
  }

  void addStoredMarker(ProfilerMarker *aStoredMarker) {
    mPendingMarkers.addStoredMarker(aStoredMarker);
  }

  void updateGeneration(int aGenID) {
    mPendingMarkers.updateGeneration(aGenID);
  }

  
  ProfilerMarkerLinkedList* getPendingMarkers()
  {
    return mPendingMarkers.getPendingMarkers();
  }

  void push(const char *aName, js::ProfileEntry::Category aCategory, uint32_t line)
  {
    push(aName, aCategory, nullptr, false, line);
  }

  void push(const char *aName, js::ProfileEntry::Category aCategory,
    void *aStackAddress, bool aCopy, uint32_t line)
  {
    if (size_t(mStackPointer) >= mozilla::ArrayLength(mStack)) {
      mStackPointer++;
      return;
    }

    volatile StackEntry &entry = mStack[mStackPointer];

    
    
    entry.setLabel(aName);
    entry.setCppFrame(aStackAddress, line);

    uint32_t uint_category = static_cast<uint32_t>(aCategory);
    MOZ_ASSERT(
      uint_category >= static_cast<uint32_t>(js::ProfileEntry::Category::FIRST) &&
      uint_category <= static_cast<uint32_t>(js::ProfileEntry::Category::LAST));

    entry.setFlag(uint_category);

    
    if (aCopy)
      entry.setFlag(js::ProfileEntry::FRAME_LABEL_COPY);
    else
      entry.unsetFlag(js::ProfileEntry::FRAME_LABEL_COPY);

    
    STORE_SEQUENCER();
    mStackPointer++;
  }
  void pop()
  {
    mStackPointer--;
  }
  bool isEmpty()
  {
    return mStackPointer == 0;
  }
  uint32_t stackSize() const
  {
    return sMin(mStackPointer, mozilla::sig_safe_t(mozilla::ArrayLength(mStack)));
  }

  void sampleRuntime(JSRuntime *runtime) {
    mRuntime = runtime;
    if (!runtime) {
      
      return;
    }

    static_assert(sizeof(mStack[0]) == sizeof(js::ProfileEntry),
                  "mStack must be binary compatible with js::ProfileEntry.");
    js::SetRuntimeProfilingStack(runtime,
                                 (js::ProfileEntry*) mStack,
                                 (uint32_t*) &mStackPointer,
                                 uint32_t(mozilla::ArrayLength(mStack)));
    if (mStartJSSampling)
      enableJSSampling();
  }
  void enableJSSampling() {
    if (mRuntime) {
      js::EnableRuntimeProfilingStack(mRuntime, true);
      js::RegisterRuntimeProfilingEventMarker(mRuntime, &ProfilerJSEventMarker);
      mStartJSSampling = false;
    } else {
      mStartJSSampling = true;
    }
  }
  void jsOperationCallback() {
    if (mStartJSSampling)
      enableJSSampling();
  }
  void disableJSSampling() {
    mStartJSSampling = false;
    if (mRuntime)
      js::EnableRuntimeProfilingStack(mRuntime, false);
  }

  
  StackEntry volatile mStack[1024];
 private:
  
  
  PendingMarkers mPendingMarkers;
  
  PendingUWTBuffers mPendingUWTBuffers;
  
  
  mozilla::sig_safe_t mStackPointer;
  
  int mSleepId;
  
  mozilla::Atomic<int> mSleepIdObserved;
  
  mozilla::Atomic<int> mSleeping;
 public:
  
  JSRuntime *mRuntime;
  
  bool mStartJSSampling;
  bool mPrivacyMode;

  enum SleepState {NOT_SLEEPING, SLEEPING_FIRST, SLEEPING_AGAIN};

  
  
  SleepState observeSleeping() {
    if (mSleeping != 0) {
      if (mSleepIdObserved == mSleepId) {
        return SLEEPING_AGAIN;
      } else {
        mSleepIdObserved = mSleepId;
        return SLEEPING_FIRST;
      }
    } else {
      return NOT_SLEEPING;
    }
  }


  
  
  void setSleeping(int sleeping) {
    MOZ_ASSERT(mSleeping != sleeping);
    mSleepId++;
    mSleeping = sleeping;
  }
};

#endif

