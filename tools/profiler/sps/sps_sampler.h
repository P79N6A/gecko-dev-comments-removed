





































#include <stdlib.h>
#include <signal.h>
#include "thread_helper.h"
#include "nscore.h"
#include "mozilla/TimeStamp.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

extern mozilla::tls::key pkey_stack;
extern mozilla::tls::key pkey_ticker;
extern bool stack_key_initialized;

#define SAMPLER_INIT() mozilla_sampler_init()
#define SAMPLER_DEINIT() mozilla_sampler_deinit()
#define SAMPLER_START(entries, interval) mozilla_sampler_start(entries, interval)
#define SAMPLER_STOP() mozilla_sampler_stop()
#define SAMPLER_IS_ACTIVE() mozilla_sampler_is_active()
#define SAMPLER_RESPONSIVENESS(time) mozilla_sampler_responsiveness(time)
#define SAMPLER_GET_RESPONSIVENESS() mozilla_sampler_get_responsiveness()
#define SAMPLER_SAVE() mozilla_sampler_save()
#define SAMPLER_GET_PROFILE() mozilla_sampler_get_profile()
#define SAMPLE_LABEL(name_space, info) mozilla::SamplerStackFrameRAII only_one_sampleraii_per_scope(FULLFUNCTION, name_space "::" info)
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






#ifdef V8_HOST_ARCH_ARM



typedef void (*LinuxKernelMemoryBarrierFunc)(void);
LinuxKernelMemoryBarrierFunc pLinuxKernelMemoryBarrier __attribute__((weak)) =
    (LinuxKernelMemoryBarrierFunc) 0xffff0fa0;

# define STORE_SEQUENCER() pLinuxKernelMemoryBarrier()
#elif defined(V8_HOST_ARCH_IA32) || defined(V8_HOST_ARCH_X64)
# if defined(_MSC_VER)
   
#  define _interlockedbittestandreset _interlockedbittestandreset_NAME_CHANGED_TO_AVOID_MSVS2005_ERROR
#  define _interlockedbittestandset _interlockedbittestandset_NAME_CHANGED_TO_AVOID_MSVS2005_ERROR
#  include <intrin.h>
   
   
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



inline void* mozilla_sampler_call_enter(const char *aInfo);
inline void  mozilla_sampler_call_exit(void* handle);
inline void  mozilla_sampler_add_marker(const char *aInfo);

void mozilla_sampler_start(int aEntries, int aInterval);
void mozilla_sampler_stop();
bool mozilla_sampler_is_active();
void mozilla_sampler_responsiveness(TimeStamp time);
const double* mozilla_sampler_get_responsiveness();
void mozilla_sampler_save();
char* mozilla_sampler_get_profile();
void mozilla_sampler_init();

namespace mozilla {

class NS_STACK_CLASS SamplerStackFrameRAII {
public:
  SamplerStackFrameRAII(const char *aFuncName, const char *aInfo) {
    mHandle = mozilla_sampler_call_enter(aInfo);
  }
  ~SamplerStackFrameRAII() {
    mozilla_sampler_call_exit(mHandle);
  }
private:
  void* mHandle;
};

} 



struct Stack
{
public:
  Stack()
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
    if (mMarkerPointer == 1024) {
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
    if (aMarkerId >= mMarkerPointer) {
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
    if (mStackPointer >= 1024) {
      mDroppedStackEntries++;
      return;
    }

    
    
    mStack[mStackPointer] = aName;
    
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

  
  char const * volatile mStack[1024];
  
  char const * volatile mMarkers[1024];
  volatile mozilla::sig_safe_t mStackPointer;
  volatile mozilla::sig_safe_t mMarkerPointer;
  volatile mozilla::sig_safe_t mDroppedStackEntries;
  
  
  volatile mozilla::sig_safe_t mQueueClearMarker;
};

inline void* mozilla_sampler_call_enter(const char *aInfo)
{
  
  
  if (!stack_key_initialized)
    return NULL;

  Stack *stack = mozilla::tls::get<Stack>(pkey_stack);
  
  
  
  
  if (!stack) {
    return stack;
  }
  stack->push(aInfo);

  
  
  
  
  
  return stack;
}

inline void mozilla_sampler_call_exit(void *aHandle)
{
  if (!aHandle)
    return;

  Stack *stack = (Stack*)aHandle;
  stack->pop();
}

inline void mozilla_sampler_add_marker(const char *aMarker)
{
  Stack *stack = mozilla::tls::get<Stack>(pkey_stack);
  if (!stack) {
    return;
  }
  stack->addMarker(aMarker);
}

