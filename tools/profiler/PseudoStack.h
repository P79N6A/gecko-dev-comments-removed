




#ifndef PROFILER_PSEUDO_STACK_H_
#define PROFILER_PSEUDO_STACK_H_

#include "mozilla/ArrayUtils.h"
#include <stdint.h>
#include "js/ProfilingStack.h"
#include <stdlib.h>
#include "mozilla/Atomics.h"
#include "nsISupportsImpl.h"



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
#  include <intrin.h>
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
class ProfilerMarker {
  friend class ProfilerLinkedList<ProfilerMarker>;
public:
  explicit ProfilerMarker(const char* aMarkerName,
         ProfilerMarkerPayload* aPayload = nullptr,
         float aTime = 0);

  ~ProfilerMarker();

  const char* GetMarkerName() const {
    return mMarkerName;
  }

  void
  StreamJSObject(JSStreamWriter& b) const;

  void SetGeneration(uint32_t aGenID);

  bool HasExpired(uint32_t aGenID) const {
    return mGenID + 2 <= aGenID;
  }

  float GetTime();

private:
  char* mMarkerName;
  ProfilerMarkerPayload* mPayload;
  ProfilerMarker* mNext;
  float mTime;
  uint32_t mGenID;
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

template<typename T>
class ProfilerSignalSafeLinkedList {
public:
  ProfilerSignalSafeLinkedList()
    : mSignalLock(false)
  {}

  ~ProfilerSignalSafeLinkedList()
  {
    if (mSignalLock) {
      
      
      abort();
    }

    while (mList.peek()) {
      delete mList.popHead();
    }
  }

  
  
  
  
  
  
  void insert(T* aElement) {
    MOZ_ASSERT(aElement);

    mSignalLock = true;
    STORE_SEQUENCER();

    mList.insert(aElement);

    STORE_SEQUENCER();
    mSignalLock = false;
  }

  
  
  
  
  ProfilerLinkedList<T>* accessList()
  {
    if (mSignalLock) {
      return nullptr;
    }
    return &mList;
  }

private:
  ProfilerLinkedList<T> mList;

  
  
  volatile bool mSignalLock;
};


void ProfilerJSEventMarker(const char *event);



struct PseudoStack
{
public:
  
  static PseudoStack *create()
  {
    return new PseudoStack();
  }

  
  void reinitializeOnResume() {
    
    
    mSleepId++;
  }

  void addMarker(const char *aMarkerStr, ProfilerMarkerPayload *aPayload, float aTime)
  {
    ProfilerMarker* marker = new ProfilerMarker(aMarkerStr, aPayload, aTime);
    mPendingMarkers.insert(marker);
  }

  
  ProfilerMarkerLinkedList* getPendingMarkers()
  {
    
    
    
    return mPendingMarkers.accessList();
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

    
    
    
    if (mStackPointer == 0) {
      ref();
    }

    volatile StackEntry &entry = mStack[mStackPointer];

    
    
    entry.setLabel(aName);
    entry.setCppFrame(aStackAddress, line);
    MOZ_ASSERT(entry.flags() == js::ProfileEntry::IS_CPP_ENTRY);

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

  
  
  
  bool popAndMaybeDelete()
  {
    mStackPointer--;
    if (mStackPointer == 0) {
      
      deref();
      return false;
    } else {
      return true;
    }
  }
  bool isEmpty()
  {
    return mStackPointer == 0;
  }
  uint32_t stackSize() const
  {
    return sMin(mStackPointer, mozilla::sig_safe_t(mozilla::ArrayLength(mStack)));
  }

  void sampleRuntime(JSRuntime* runtime) {
    if (mRuntime && !runtime) {
      
      
      flushSamplerOnJSShutdown();
    }

    mRuntime = runtime;

    if (!runtime) {
      return;
    }

    static_assert(sizeof(mStack[0]) == sizeof(js::ProfileEntry),
                  "mStack must be binary compatible with js::ProfileEntry.");
    js::SetRuntimeProfilingStack(runtime,
                                 (js::ProfileEntry*) mStack,
                                 (uint32_t*) &mStackPointer,
                                 (uint32_t) mozilla::ArrayLength(mStack));
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

  
  PseudoStack()
    : mStackPointer(0)
    , mSleepId(0)
    , mSleepIdObserved(0)
    , mSleeping(false)
    , mRefCnt(1)
    , mRuntime(nullptr)
    , mStartJSSampling(false)
    , mPrivacyMode(false)
  { }

  
  ~PseudoStack() {
    if (mStackPointer != 0) {
      
      
      
      
      abort();
    }
  }

  
  PseudoStack(const PseudoStack&) = delete;
  void operator=(const PseudoStack&) = delete;

  void flushSamplerOnJSShutdown();

  
  
  ProfilerSignalSafeLinkedList<ProfilerMarker> mPendingMarkers;
  
  
  mozilla::sig_safe_t mStackPointer;
  
  int mSleepId;
  
  mozilla::Atomic<int> mSleepIdObserved;
  
  mozilla::Atomic<int> mSleeping;
  
  
  
  mozilla::Atomic<int> mRefCnt;

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

  void ref() {
    ++mRefCnt;
  }

  void deref() {
    int newValue = --mRefCnt;
    if (newValue == 0) {
      delete this;
    }
  }
};

#endif

