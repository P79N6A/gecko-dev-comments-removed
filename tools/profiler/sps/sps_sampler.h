





































#include <pthread.h>
#include "base/atomicops.h"
#include "nscore.h"
#include "mozilla/TimeStamp.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;



extern pthread_key_t pkey_stack;

#define SAMPLER_INIT() mozilla_sampler_init();
#define SAMPLER_DEINIT() mozilla_sampler_deinit();
#define SAMPLER_START(entries, interval) mozilla_sampler_start(entries, interval);
#define SAMPLER_STOP() mozilla_sampler_stop();
#define SAMPLER_IS_ACTIVE() mozilla_sampler_is_active();
#define SAMPLER_RESPONSIVENESS(time) mozilla_sampler_responsiveness(time)
#define SAMPLER_GET_RESPONSIVENESS() mozilla_sampler_get_responsiveness()
#define SAMPLER_SAVE() mozilla_sampler_save();
#define SAMPLER_GET_PROFILE() mozilla_sampler_get_profile();
#define SAMPLE_LABEL(name_space, info) mozilla::SamplerStackFrameRAII only_one_sampleraii_per_scope(FULLFUNCTION, name_space "::" info);
#define SAMPLE_MARKER(info) mozilla_sampler_add_marker(info);





#ifdef ARCH_CPU_ARM_FAMILY



# define STORE_SEQUENCER() base::subtle::MemoryBarrier();
#elif ARCH_CPU_X86_FAMILY
# define STORE_SEQUENCER() asm volatile("" ::: "memory");
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
const float* mozilla_sampler_get_responsiveness();
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
    
    asm("":::"memory");
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

  
  const char *mStack[1024];
  
  const char *mMarkers[1024];
  sig_atomic_t mStackPointer;
  sig_atomic_t mMarkerPointer;
  sig_atomic_t mDroppedStackEntries;
  
  
  sig_atomic_t mQueueClearMarker;
};

inline void* mozilla_sampler_call_enter(const char *aInfo)
{
  Stack *stack = (Stack*)pthread_getspecific(pkey_stack);
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
  Stack *stack = (Stack*)pthread_getspecific(pkey_stack);
  if (!stack) {
    return;
  }
  stack->addMarker(aMarker);
}

