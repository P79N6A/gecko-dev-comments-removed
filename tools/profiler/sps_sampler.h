




#ifndef TOOLS_SPS_SAMPLER_H_
#define TOOLS_SPS_SAMPLER_H_

#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include "mozilla/ThreadLocal.h"
#include "nscore.h"
#include "mozilla/Assertions.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Util.h"
#include "nsAlgorithm.h"
#include <algorithm>





#ifdef MOZ_WIDGET_QT
#undef slots
#endif
#include "jsfriendapi.h"


#ifdef min
#undef min
#endif

using mozilla::TimeStamp;
using mozilla::TimeDuration;

struct PseudoStack;
class TableTicker;
class JSCustomObject;

extern mozilla::ThreadLocal<PseudoStack *> tlsPseudoStack;
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



inline void* mozilla_sampler_call_enter(const char *aInfo, void *aFrameAddress = NULL,
                                        bool aCopy = false, uint32_t line = 0);
inline void  mozilla_sampler_call_exit(void* handle);
inline void  mozilla_sampler_add_marker(const char *aInfo);

void mozilla_sampler_start1(int aEntries, int aInterval, const char** aFeatures,
                            uint32_t aFeatureCount);
void mozilla_sampler_start2(int aEntries, int aInterval, const char** aFeatures,
                            uint32_t aFeatureCount);

void mozilla_sampler_stop1();
void mozilla_sampler_stop2();

bool mozilla_sampler_is_active1();
bool mozilla_sampler_is_active2();

void mozilla_sampler_responsiveness1(TimeStamp time);
void mozilla_sampler_responsiveness2(TimeStamp time);

void mozilla_sampler_frame_number1(int frameNumber);
void mozilla_sampler_frame_number2(int frameNumber);

const double* mozilla_sampler_get_responsiveness1();
const double* mozilla_sampler_get_responsiveness2();

void mozilla_sampler_save1();
void mozilla_sampler_save2();

char* mozilla_sampler_get_profile1();
char* mozilla_sampler_get_profile2();

JSObject *mozilla_sampler_get_profile_data1(JSContext *aCx);
JSObject *mozilla_sampler_get_profile_data2(JSContext *aCx);

const char** mozilla_sampler_get_features1();
const char** mozilla_sampler_get_features2();

void mozilla_sampler_init1();
void mozilla_sampler_init2();

void mozilla_sampler_shutdown1();
void mozilla_sampler_shutdown2();

void mozilla_sampler_print_location1();
void mozilla_sampler_print_location2();




void mozilla_sampler_lock1();
void mozilla_sampler_lock2();


void mozilla_sampler_unlock1();
void mozilla_sampler_unlock2();


extern bool sps_version2();

static inline
void profiler_init()
{
  if (!sps_version2()) {
    mozilla_sampler_init1();
  } else {
    mozilla_sampler_init2();
  }
}

static inline
void profiler_shutdown()
{
  if (!sps_version2()) {
    mozilla_sampler_shutdown1();
  } else {
    mozilla_sampler_shutdown2();
  }
}

static inline
void profiler_start(int aProfileEntries, int aInterval,
                       const char** aFeatures, uint32_t aFeatureCount)
{
  if (!sps_version2()) {
    mozilla_sampler_start1(aProfileEntries, aInterval, aFeatures, aFeatureCount);
  } else {
    mozilla_sampler_start2(aProfileEntries, aInterval, aFeatures, aFeatureCount);
  }
}

static inline
void profiler_stop()
{
  if (!sps_version2()) {
    mozilla_sampler_stop1();
  } else {
    mozilla_sampler_stop2();
  }
}

static inline
bool profiler_is_active()
{
  if (!sps_version2()) {
    return mozilla_sampler_is_active1();
  } else {
    return mozilla_sampler_is_active2();
  }
}

static inline
void profiler_responsiveness(const TimeStamp& aTime)
{
  if (!sps_version2()) {
    mozilla_sampler_responsiveness1(aTime);
  } else {
    mozilla_sampler_responsiveness2(aTime);
  }
}

static inline
const double* profiler_get_responsiveness()
{
  if (!sps_version2()) {
    return mozilla_sampler_get_responsiveness1();
  } else {
    return mozilla_sampler_get_responsiveness2();
  }
}

static inline
void profiler_set_frame_number(int frameNumber)
{
  if (!sps_version2()) {
    return mozilla_sampler_frame_number1(frameNumber);
  } else {
    return mozilla_sampler_frame_number2(frameNumber);
  }
}

static inline
char* profiler_get_profile()
{
  if (!sps_version2()) {
    return mozilla_sampler_get_profile1();
  } else {
    return mozilla_sampler_get_profile2();
  }
}

static inline
const char** profiler_get_features()
{
  if (!sps_version2()) {
    return mozilla_sampler_get_features1();
  } else {
    return mozilla_sampler_get_features2();
  }
}

static inline
void profiler_print_location()
{
  if (!sps_version2()) {
    return mozilla_sampler_print_location1();
  } else {
    return mozilla_sampler_print_location2();
  }
}

static inline
void profiler_lock()
{
  if (!sps_version2()) {
    return mozilla_sampler_lock1();
  } else {
    return mozilla_sampler_lock2();
  }
}

static inline
void profiler_unlock()
{
  if (!sps_version2()) {
    return mozilla_sampler_unlock1();
  } else {
    return mozilla_sampler_unlock2();
  }
}




#define SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line) id ## line
#define SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, line) SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line)
#define SAMPLER_APPEND_LINE_NUMBER(id) SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, __LINE__)

#define PROFILER_LABEL(name_space, info) mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__)
#define PROFILER_LABEL_PRINTF(name_space, info, ...) mozilla::SamplerStackFramePrintfRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__, __VA_ARGS__)
#define PROFILER_MARKER(info) mozilla_sampler_add_marker(info)
#define PROFILER_MAIN_THREAD_LABEL(name_space, info)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__)
#define PROFILER_MAIN_THREAD_LABEL_PRINTF(name_space, info, ...)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla::SamplerStackFramePrintfRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__, __VA_ARGS__)
#define PROFILER_MAIN_THREAD_MARKER(info)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla_sampler_add_marker(info)



#if defined(_M_X64) || defined(__x86_64__)
#define V8_HOST_ARCH_X64 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define V8_HOST_ARCH_IA32 1
#elif defined(__ARMEL__)
#define V8_HOST_ARCH_ARM 1
#else
#warning Please add support for your architecture in chromium_types.h
#endif




#ifdef MOZ_WIDGET_GONK
# define PLATFORM_LIKELY_MEMORY_CONSTRAINED
#endif

#if !defined(PLATFORM_LIKELY_MEMORY_CONSTRAINED) && !defined(ARCH_ARMV6)
# define PROFILE_DEFAULT_ENTRY 1000000
#else
# define PROFILE_DEFAULT_ENTRY 100000
#endif

#if defined(PLATFORM_LIKELY_MEMORY_CONSTRAINED)





# define PROFILE_DEFAULT_INTERVAL 10
#elif defined(ANDROID)









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

namespace mozilla {

class NS_STACK_CLASS SamplerStackFrameRAII {
public:
  
  SamplerStackFrameRAII(const char *aInfo, uint32_t line) {
    mHandle = mozilla_sampler_call_enter(aInfo, this, false, line);
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
  
  SamplerStackFramePrintfRAII(const char *aDefault, uint32_t line, const char *aFormat, ...) {
    if (profiler_is_active()) {
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
      mHandle = mozilla_sampler_call_enter(mDest, this, true, line);
      va_end(args);
    } else {
      mHandle = mozilla_sampler_call_enter(aDefault, NULL, false, line);
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



struct PseudoStack
{
public:
  PseudoStack()
    : mStackPointer(0)
    , mSignalLock(false)
    , mMarkerPointer(0)
    , mQueueClearMarker(false)
    , mRuntime(NULL)
    , mStartJSSampling(false)
  { }

  void addMarker(const char *aMarker)
  {
    char* markerCopy = strdup(aMarker);
    mSignalLock = true;
    STORE_SEQUENCER();

    if (mQueueClearMarker) {
      clearMarkers();
    }
    if (!aMarker) {
      return; 
    }
    if (size_t(mMarkerPointer) == mozilla::ArrayLength(mMarkers)) {
      return; 
    }
    mMarkers[mMarkerPointer] = markerCopy;
    mMarkerPointer++;

    mSignalLock = false;
    STORE_SEQUENCER();
  }

  
  const char* getMarker(int aMarkerId)
  {
    
    
    
    
    
    
    
    if (mSignalLock || mQueueClearMarker || aMarkerId < 0 ||
      static_cast<mozilla::sig_safe_t>(aMarkerId) >= mMarkerPointer) {
      return NULL;
    }
    return mMarkers[aMarkerId];
  }

  
  void clearMarkers()
  {
    for (mozilla::sig_safe_t i = 0; i < mMarkerPointer; i++) {
      free(mMarkers[i]);
    }
    mMarkerPointer = 0;
    mQueueClearMarker = false;
  }

  void push(const char *aName, uint32_t line)
  {
    push(aName, NULL, false, line);
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
    return std::min<uint32_t>(mStackPointer, mozilla::ArrayLength(mStack));
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
                                 mozilla::ArrayLength(mStack));
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
  void disableJSSampling() {
    mStartJSSampling = false;
    if (mRuntime)
      js::EnableRuntimeProfilingStack(mRuntime, false);
  }

  
  StackEntry volatile mStack[1024];
  
  char* mMarkers[1024];
 private:
  
  
  mozilla::sig_safe_t mStackPointer;
  
  volatile bool mSignalLock;
 public:
  volatile mozilla::sig_safe_t mMarkerPointer;
  
  
  volatile mozilla::sig_safe_t mQueueClearMarker;
  
  JSRuntime *mRuntime;
  
  bool mStartJSSampling;
};

inline PseudoStack* mozilla_get_pseudo_stack(void)
{
  if (!stack_key_initialized)
    return NULL;
  return tlsPseudoStack.get();
}

inline void* mozilla_sampler_call_enter(const char *aInfo, void *aFrameAddress,
                                        bool aCopy, uint32_t line)
{
  
  
  if (!stack_key_initialized)
    return NULL;

  PseudoStack *stack = tlsPseudoStack.get();
  
  
  
  
  if (!stack) {
    return stack;
  }
  stack->push(aInfo, aFrameAddress, aCopy, line);

  
  
  
  
  
  return stack;
}

inline void mozilla_sampler_call_exit(void *aHandle)
{
  if (!aHandle)
    return;

  PseudoStack *stack = (PseudoStack*)aHandle;
  stack->pop();
}

inline void mozilla_sampler_add_marker(const char *aMarker)
{
  if (!stack_key_initialized)
    return;

  
  
  if (!profiler_is_active()) {
    return;
  }

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    return;
  }
  stack->addMarker(aMarker);
}

#endif 
