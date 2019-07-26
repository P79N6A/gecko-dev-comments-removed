





#ifndef TOOLS_SPS_SAMPLER_H_
#define TOOLS_SPS_SAMPLER_H_

#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <algorithm>
#include "mozilla/ThreadLocal.h"
#include "mozilla/Assertions.h"
#include "mozilla/Util.h"
#include "nsAlgorithm.h"
#include "nscore.h"
#include "GeckoProfilerFunc.h"
#include "PseudoStack.h"
#include "nsISupports.h"




#ifdef MOZ_WIDGET_QT
#undef slots
#endif


#ifdef min
#undef min
#endif

class TableTicker;
class JSCustomObject;

namespace mozilla {
class TimeStamp;
}

extern mozilla::ThreadLocal<PseudoStack *> tlsPseudoStack;
extern mozilla::ThreadLocal<TableTicker *> tlsTicker;
extern mozilla::ThreadLocal<void *> tlsStackTop;
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

static inline
void profiler_init(void* stackTop)
{
  mozilla_sampler_init(stackTop);
}

static inline
void profiler_shutdown()
{
  mozilla_sampler_shutdown();
}

static inline
void profiler_start(int aProfileEntries, int aInterval,
                       const char** aFeatures, uint32_t aFeatureCount,
                       const char** aThreadNameFilters, uint32_t aFilterCount)
{
  mozilla_sampler_start(aProfileEntries, aInterval, aFeatures, aFeatureCount, aThreadNameFilters, aFilterCount);
}

static inline
void profiler_stop()
{
  mozilla_sampler_stop();
}

static inline
ProfilerBacktrace* profiler_get_backtrace()
{
  return mozilla_sampler_get_backtrace();
}

static inline
void profiler_free_backtrace(ProfilerBacktrace* aBacktrace)
{
  mozilla_sampler_free_backtrace(aBacktrace);
}

static inline
bool profiler_is_active()
{
  return mozilla_sampler_is_active();
}

static inline
void profiler_responsiveness(const mozilla::TimeStamp& aTime)
{
  mozilla_sampler_responsiveness(aTime);
}

static inline
const double* profiler_get_responsiveness()
{
  return mozilla_sampler_get_responsiveness();
}

static inline
void profiler_set_frame_number(int frameNumber)
{
  return mozilla_sampler_frame_number(frameNumber);
}

static inline
char* profiler_get_profile()
{
  return mozilla_sampler_get_profile();
}

static inline
JSObject* profiler_get_profile_jsobject(JSContext* aCx)
{
  return mozilla_sampler_get_profile_data(aCx);
}

static inline
const char** profiler_get_features()
{
  return mozilla_sampler_get_features();
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
  return mozilla_sampler_lock();
}

static inline
void profiler_unlock()
{
  return mozilla_sampler_unlock();
}

static inline
void profiler_register_thread(const char* name, void* stackTop)
{
  mozilla_sampler_register_thread(name, stackTop);
}

static inline
void profiler_unregister_thread()
{
  mozilla_sampler_unregister_thread();
}

static inline
void profiler_js_operation_callback()
{
  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    return;
  }

  stack->jsOperationCallback();
}

static inline
double profiler_time()
{
  return mozilla_sampler_time();
}

static inline
double profiler_time(const mozilla::TimeStamp& aTime)
{
  return mozilla_sampler_time(aTime);
}

static inline
bool profiler_in_privacy_mode()
{
  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    return false;
  }
  return stack->mPrivacyMode;
}

static inline void profiler_tracing(const char* aCategory, const char* aInfo,
                                    TracingMetadata aMetaData = TRACING_DEFAULT)
{
  if (!stack_key_initialized)
    return;

  
  
  if (!profiler_is_active()) {
    return;
  }

  mozilla_sampler_tracing(aCategory, aInfo, aMetaData);
}




#define SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line) id ## line
#define SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, line) SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line)
#define SAMPLER_APPEND_LINE_NUMBER(id) SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, __LINE__)

#define PROFILER_LABEL(name_space, info) mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__)
#define PROFILER_LABEL_PRINTF(name_space, info, ...) mozilla::SamplerStackFramePrintfRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__, __VA_ARGS__)

#define PROFILER_MARKER(info) mozilla_sampler_add_marker(info)
#define PROFILER_MARKER_PAYLOAD(info, payload) mozilla_sampler_add_marker(info, payload)
#define PROFILER_MAIN_THREAD_MARKER(info)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla_sampler_add_marker(info)

#define PROFILER_MAIN_THREAD_LABEL(name_space, info)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__)
#define PROFILER_MAIN_THREAD_LABEL_PRINTF(name_space, info, ...)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla::SamplerStackFramePrintfRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, __LINE__, __VA_ARGS__)





#ifdef MOZ_WIDGET_GONK
# define PLATFORM_LIKELY_MEMORY_CONSTRAINED
#endif

#if !defined(PLATFORM_LIKELY_MEMORY_CONSTRAINED) && !defined(ARCH_ARMV6)
# define PROFILE_DEFAULT_ENTRY 1000000
#else
# define PROFILE_DEFAULT_ENTRY 100000
#endif



#define GET_BACKTRACE_DEFAULT_ENTRY 1000

#if defined(PLATFORM_LIKELY_MEMORY_CONSTRAINED)





# define PROFILE_DEFAULT_INTERVAL 10
#elif defined(ANDROID)









#define PROFILE_DEFAULT_INTERVAL 1
#else
#define PROFILE_DEFAULT_INTERVAL 1
#endif
#define PROFILE_DEFAULT_FEATURES NULL
#define PROFILE_DEFAULT_FEATURE_COUNT 0

namespace mozilla {

class MOZ_STACK_CLASS SamplerStackFrameRAII {
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
class MOZ_STACK_CLASS SamplerStackFramePrintfRAII {
public:
  
  SamplerStackFramePrintfRAII(const char *aDefault, uint32_t line, const char *aFormat, ...) {
    if (profiler_is_active() && !profiler_in_privacy_mode()) {
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

inline void mozilla_sampler_add_marker(const char *aMarker, ProfilerMarkerPayload *aPayload)
{
  if (!stack_key_initialized)
    return;

  
  
  if (!profiler_is_active()) {
    return;
  }

  
  if (profiler_in_privacy_mode()) {
    return;
  }

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    return;
  }
  stack->addMarker(aMarker, aPayload);
}

#endif 
