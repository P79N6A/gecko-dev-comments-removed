




#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include "mozilla/ThreadLocal.h"
#include "nscore.h"
#include "jsapi.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Util.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

struct ProfileStack;
class TableTicker;

extern mozilla::ThreadLocal<ProfileStack *> tlsStack;
extern mozilla::ThreadLocal<TableTicker *> tlsTicker;
extern bool stack_key_initialized;

#ifndef SAMPLE_FUNCTION_NAME
# ifdef __GNUC__
#  define SAMPLE_FUNCTION_NAME __FUNCTION__
# elif defined(_MSC_VER)
#  define SAMPLE_FUNCTION_NAME __FUNCTION__
# else
#  define SAMPLE_FUNCTION_NAME __func__  // defined in C99, supported in various C++ compilers. Just raw function name.
# endif
#endif

#define SAMPLER_INIT() mozilla_sampler_init()
#define SAMPLER_DEINIT() mozilla_sampler_deinit()
#define SAMPLER_START(entries, interval, features, featureCount) mozilla_sampler_start(entries, interval, features, featureCount)
#define SAMPLER_STOP() mozilla_sampler_stop()
#define SAMPLER_IS_ACTIVE() mozilla_sampler_is_active()
#define SAMPLER_RESPONSIVENESS(time) mozilla_sampler_responsiveness(time)
#define SAMPLER_GET_RESPONSIVENESS() mozilla_sampler_get_responsiveness()
#define SAMPLER_SAVE() mozilla_sampler_save()
#define SAMPLER_GET_PROFILE() mozilla_sampler_get_profile()
#define SAMPLER_GET_PROFILE_DATA(ctx) mozilla_sampler_get_profile_data(ctx)
#define SAMPLER_GET_FEATURES() mozilla_sampler_get_features()



#define SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line) id ## line
#define SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, line) SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line)
#define SAMPLER_APPEND_LINE_NUMBER(id) SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, __LINE__)

#define SAMPLE_LABEL(name_space, info) mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info)
#define SAMPLE_LABEL_PRINTF(name_space, info, format, ...) mozilla::SamplerStackFramePrintfRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, format, __VA_ARGS__)
#define SAMPLE_MARKER(info) mozilla_sampler_add_marker(info)



#if defined(_M_X64) || defined(__x86_64__)
#define V8_HOST_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define V8_HOST_ARCH_IA32 1
#elif defined(__ARMEL__)
#define V8_HOST_ARCH_ARM 1
#else
#warning Please add support for your architecture in chromium_types.h
#endif

#define PROFILE_DEFAULT_ENTRY 100000
#ifdef ANDROID









#define PROFILE_DEFAULT_INTERVAL 1
#else
#define PROFILE_DEFAULT_INTERVAL 1
#endif
#define PROFILE_DEFAULT_FEATURES NULL
#define PROFILE_DEFAULT_FEATURE_COUNT 0





#ifdef V8_HOST_ARCH_ARM



typedef void (*LinuxKernelMemoryBarrierFunc)(void);
LinuxKernelMemoryBarrierFunc pLinuxKernelMemoryBarrier __attribute__((weak)) =
    (LinuxKernelMemoryBarrierFunc) 0xffff0fa0;

# define STORE_SEQUENCER() pLinuxKernelMemoryBarrier()
#elif defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_X64)
# if defined(_MSC_VER)
    
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



inline void* mozilla_sampler_call_enter(const char *aInfo, void *aFrameAddress = NULL, bool aCopy = false);
inline void  mozilla_sampler_call_exit(void* handle);
inline void  mozilla_sampler_add_marker(const char *aInfo);

void mozilla_sampler_start(int aEntries, int aInterval, const char** aFeatures, uint32_t aFeatureCount);
void mozilla_sampler_stop();
bool mozilla_sampler_is_active();
void mozilla_sampler_responsiveness(TimeStamp time);
const double* mozilla_sampler_get_responsiveness();
void mozilla_sampler_save();
char* mozilla_sampler_get_profile();
JSObject *mozilla_sampler_get_profile_data(JSContext *aCx);
const char** mozilla_sampler_get_features();
void mozilla_sampler_init();

namespace mozilla {

class NS_STACK_CLASS SamplerStackFrameRAII {
public:
  
  SamplerStackFrameRAII(const char *aInfo) {
    mHandle = mozilla_sampler_call_enter(aInfo);
  }
  ~SamplerStackFrameRAII() {
    mozilla_sampler_call_exit(mHandle);
  }
private:
  void* mHandle;
};

static const int SAMPLER_MAX_STRING = 128;
class NS_STACK_CLASS SamplerStackFramePrintfRAII {
public:
  
  SamplerStackFramePrintfRAII(const char *aDefault, const char *aFormat, ...) {
    if (mozilla_sampler_is_active()) {
      va_list args;
      va_start(args, aFormat);
      char buff[SAMPLER_MAX_STRING];

      
      
#if _MSC_VER
      _vsnprintf(buff, SAMPLER_MAX_STRING, aFormat, args);
      _snprintf(mDest, SAMPLER_MAX_STRING, "%s %s", aDefault, buff);
#else
      vsnprintf(buff, SAMPLER_MAX_STRING, aFormat, args);
      snprintf(mDest, SAMPLER_MAX_STRING, "%s %s", aDefault, buff);
#endif
      mHandle = mozilla_sampler_call_enter(mDest, this, true);
      va_end(args);
    } else {
      mHandle = mozilla_sampler_call_enter(aDefault);
    }
  }
  ~SamplerStackFramePrintfRAII() {
    mozilla_sampler_call_exit(mHandle);
  }
private:
  char mDest[SAMPLER_MAX_STRING];
  void* mHandle;
};

} 

class StackEntry
{
public:
  
  
  static const void* EncodeStackAddress(const void* aStackAddress, bool aCopy) {
    aStackAddress = reinterpret_cast<const void*>(
                      reinterpret_cast<uintptr_t>(aStackAddress) & ~0x1);
    if (!aCopy)
      aStackAddress = reinterpret_cast<const void*>(
                        reinterpret_cast<uintptr_t>(aStackAddress) | 0x1);
    return aStackAddress;
  }

  bool isCopyLabel() const volatile {
    return !((uintptr_t)mStackAddress & 0x1);
  }

  const char* mLabel;
  
  
  
  
  const void* mStackAddress;
};



struct ProfileStack
{
public:
  ProfileStack()
    : mStackPointer(0)
    , mMarkerPointer(0)
    , mDroppedStackEntries(0)
    , mQueueClearMarker(false)
  { }

  void addMarker(const char *aMarker)
  {
    if (mQueueClearMarker) {
      clearMarkers();
    }
    if (!aMarker) {
      return; 
    }
    if (size_t(mMarkerPointer) == mozilla::ArrayLength(mMarkers)) {
      return; 
    }
    mMarkers[mMarkerPointer] = aMarker;
    STORE_SEQUENCER();
    mMarkerPointer++;
  }

  
  const char* getMarker(int aMarkerId)
  {
    if (mQueueClearMarker) {
      clearMarkers();
    }
    if (aMarkerId < 0 ||
      static_cast<mozilla::sig_safe_t>(aMarkerId) >= mMarkerPointer) {
      return NULL;
    }
    return mMarkers[aMarkerId];
  }

  
  void clearMarkers()
  {
    mMarkerPointer = 0;
    mQueueClearMarker = false;
  }

  void push(const char *aName)
  {
    push(aName, NULL, false);
  }

  void push(const char *aName, void *aStackAddress, bool aCopy)
  {
    if (size_t(mStackPointer) >= mozilla::ArrayLength(mStack)) {
      mDroppedStackEntries++;
      return;
    }

    
    
    mStack[mStackPointer].mLabel = aName;
    mStack[mStackPointer].mStackAddress = StackEntry::EncodeStackAddress(aStackAddress, aCopy);

    
    STORE_SEQUENCER();
    mStackPointer++;
  }
  void pop()
  {
    if (mDroppedStackEntries > 0) {
      mDroppedStackEntries--;
    } else {
      mStackPointer--;
    }
  }
  bool isEmpty()
  {
    return mStackPointer == 0;
  }

  
  StackEntry volatile mStack[1024];
  
  char const * volatile mMarkers[1024];
  volatile mozilla::sig_safe_t mStackPointer;
  volatile mozilla::sig_safe_t mMarkerPointer;
  volatile mozilla::sig_safe_t mDroppedStackEntries;
  
  
  volatile mozilla::sig_safe_t mQueueClearMarker;
};

inline void* mozilla_sampler_call_enter(const char *aInfo, void *aFrameAddress, bool aCopy)
{
  
  
  if (!stack_key_initialized)
    return NULL;

  ProfileStack *stack = tlsStack.get();
  
  
  
  
  if (!stack) {
    return stack;
  }
  stack->push(aInfo, aFrameAddress, aCopy);

  
  
  
  
  
  return stack;
}

inline void mozilla_sampler_call_exit(void *aHandle)
{
  if (!aHandle)
    return;

  ProfileStack *stack = (ProfileStack*)aHandle;
  stack->pop();
}

inline void mozilla_sampler_add_marker(const char *aMarker)
{
  ProfileStack *stack = tlsStack.get();
  if (!stack) {
    return;
  }
  stack->addMarker(aMarker);
}

