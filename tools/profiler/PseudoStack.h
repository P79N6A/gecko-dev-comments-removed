




#ifndef PROFILER_PSEUDO_STACK_H_
#define PROFILER_PSEUDO_STACK_H_

#include "mozilla/NullPtr.h"
#include <stdint.h>
#include "js/ProfilingStack.h"
#include <stdlib.h>
#include <algorithm>



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









class StackEntry : public js::ProfileEntry
{
public:

  bool isCopyLabel() volatile {
    return !((uintptr_t)stackAddress() & 0x1);
  }

  void setStackAddressCopy(void *sparg, bool copy) volatile {
    
    
    
    
    
    if (copy) {
      setStackAddress(reinterpret_cast<void*>(
                        reinterpret_cast<uintptr_t>(sparg) & ~0x1));
    } else {
      setStackAddress(reinterpret_cast<void*>(
                        reinterpret_cast<uintptr_t>(sparg) | 0x1));
    }
  }
};

class ProfilerMarkerPayload;
class ProfilerMarkerLinkedList;
class JSAObjectBuilder;
class JSCustomArray;
class ThreadProfile;
class ProfilerMarker {
  friend class ProfilerMarkerLinkedList;
public:
  ProfilerMarker(const char* aMarkerName,
         ProfilerMarkerPayload* aPayload = nullptr);

  ~ProfilerMarker();

  const char* GetMarkerName() const {
    return mMarkerName;
  }

  template<typename Builder> void
  BuildJSObject(Builder& b, typename Builder::ArrayHandle markers) const;

  void SetGeneration(int aGenID);

  bool HasExpired(int aGenID) const {
    return mGenID + 2 <= aGenID;
  }

private:
  char* mMarkerName;
  ProfilerMarkerPayload* mPayload;
  ProfilerMarker* mNext;
  int mGenID;
};

class ProfilerMarkerLinkedList {
public:
  ProfilerMarkerLinkedList()
    : mHead(nullptr)
    , mTail(nullptr)
  {}

  void insert(ProfilerMarker* elem);
  ProfilerMarker* popHead();

  const ProfilerMarker* peek() {
    return mHead;
  }

private:
  ProfilerMarker* mHead;
  ProfilerMarker* mTail;
};

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



struct PseudoStack
{
public:
  PseudoStack()
    : mStackPointer(0)
    , mRuntime(nullptr)
    , mStartJSSampling(false)
    , mPrivacyMode(false)
  { }

  ~PseudoStack() {
    if (mStackPointer != 0) {
      
      
      
      
      abort();
    }
  }

  void addMarker(const char *aMarkerStr, ProfilerMarkerPayload *aPayload)
  {
    ProfilerMarker* marker = new ProfilerMarker(aMarkerStr, aPayload);
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

  void push(const char *aName, uint32_t line)
  {
    push(aName, nullptr, false, line);
  }

  void push(const char *aName, void *aStackAddress, bool aCopy, uint32_t line)
  {
    if (size_t(mStackPointer) >= mozilla::ArrayLength(mStack)) {
      mStackPointer++;
      return;
    }

    
    
    mStack[mStackPointer].setLabel(aName);
    mStack[mStackPointer].setStackAddressCopy(aStackAddress, aCopy);
    mStack[mStackPointer].setLine(line);

    
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
    return std::min<uint32_t>(mStackPointer, mozilla::sig_safe_t(mozilla::ArrayLength(mStack)));
  }

  void sampleRuntime(JSRuntime *runtime) {
    mRuntime = runtime;
    if (!runtime) {
      
      return;
    }

    JS_STATIC_ASSERT(sizeof(mStack[0]) == sizeof(js::ProfileEntry));
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
  
  
  mozilla::sig_safe_t mStackPointer;
 public:
  
  JSRuntime *mRuntime;
  
  bool mStartJSSampling;
  bool mPrivacyMode;
};

#endif

