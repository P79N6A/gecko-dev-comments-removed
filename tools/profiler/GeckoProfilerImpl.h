





#ifndef TOOLS_SPS_SAMPLER_H_
#define TOOLS_SPS_SAMPLER_H_

#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include "mozilla/ThreadLocal.h"
#include "mozilla/Assertions.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/UniquePtr.h"
#include "nscore.h"
#include "GeckoProfilerFunc.h"
#include "PseudoStack.h"
#include "nsISupports.h"
#include "ProfilerBacktrace.h"

#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
#endif




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
#ifdef MOZ_TASK_TRACER
  mozilla::tasktracer::InitTaskTracer();
#endif
  mozilla_sampler_init(stackTop);
}

static inline
void profiler_shutdown()
{
#ifdef MOZ_TASK_TRACER
  mozilla::tasktracer::ShutdownTaskTracer();
#endif
  mozilla_sampler_shutdown();
}

static inline
void profiler_start(int aProfileEntries, double aInterval,
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
bool profiler_is_paused()
{
  return mozilla_sampler_is_paused();
}

static inline
void profiler_pause()
{
  mozilla_sampler_pause();
}

static inline
void profiler_resume()
{
  mozilla_sampler_resume();
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
bool profiler_feature_active(const char* aName)
{
  return mozilla_sampler_feature_active(aName);
}

static inline
void profiler_responsiveness(const mozilla::TimeStamp& aTime)
{
  mozilla_sampler_responsiveness(aTime);
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
void profiler_save_profile_to_file(const char* aFilename)
{
  return mozilla_sampler_save_profile_to_file(aFilename);
}

static inline
const char** profiler_get_features()
{
  return mozilla_sampler_get_features();
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
void profiler_sleep_start()
{
  mozilla_sampler_sleep_start();
}

static inline
void profiler_sleep_end()
{
  mozilla_sampler_sleep_end();
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
                                    ProfilerBacktrace* aCause,
                                    TracingMetadata aMetaData = TRACING_DEFAULT)
{
  
  
  if (!stack_key_initialized || !profiler_is_active()) {
    delete aCause;
    return;
  }

  mozilla_sampler_tracing(aCategory, aInfo, aCause, aMetaData);
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




#ifdef MOZ_USE_SYSTRACE
#ifndef ATRACE_TAG
# define ATRACE_TAG ATRACE_TAG_ALWAYS
#endif


# ifndef HAVE_ANDROID_OS
#   define HAVE_ANDROID_OS
#   define REMOVE_HAVE_ANDROID_OS
# endif






# undef _LIBS_CUTILS_TRACE_H
# include <utils/Trace.h>
# define MOZ_PLATFORM_TRACING(name) ATRACE_NAME(name);
# ifdef REMOVE_HAVE_ANDROID_OS
#  undef HAVE_ANDROID_OS
#  undef REMOVE_HAVE_ANDROID_OS
# endif
#else
# define MOZ_PLATFORM_TRACING(name)
#endif




#define SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line) id ## line
#define SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, line) SAMPLER_APPEND_LINE_NUMBER_PASTE(id, line)
#define SAMPLER_APPEND_LINE_NUMBER(id) SAMPLER_APPEND_LINE_NUMBER_EXPAND(id, __LINE__)

#define PROFILER_LABEL(name_space, info, category) MOZ_PLATFORM_TRACING(name_space "::" info) mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, category, __LINE__)
#define PROFILER_LABEL_FUNC(category) MOZ_PLATFORM_TRACING(SAMPLE_FUNCTION_NAME) mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(SAMPLE_FUNCTION_NAME, category, __LINE__)
#define PROFILER_LABEL_PRINTF(name_space, info, category, ...) MOZ_PLATFORM_TRACING(name_space "::" info) mozilla::SamplerStackFramePrintfRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, category, __LINE__, __VA_ARGS__)

#define PROFILER_MARKER(info) mozilla_sampler_add_marker(info)
#define PROFILER_MARKER_PAYLOAD(info, payload) mozilla_sampler_add_marker(info, payload)
#define PROFILER_MAIN_THREAD_MARKER(info)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla_sampler_add_marker(info)

#define PROFILER_MAIN_THREAD_LABEL(name_space, info, category)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla::SamplerStackFrameRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, category, __LINE__)
#define PROFILER_MAIN_THREAD_LABEL_PRINTF(name_space, info, category, ...)  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread"); mozilla::SamplerStackFramePrintfRAII SAMPLER_APPEND_LINE_NUMBER(sampler_raii)(name_space "::" info, category, __LINE__, __VA_ARGS__)





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

class MOZ_STACK_CLASS GeckoProfilerTracingRAII {
public:
  GeckoProfilerTracingRAII(const char* aCategory, const char* aInfo,
                           mozilla::UniquePtr<ProfilerBacktrace> aBacktrace
                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mCategory(aCategory)
    , mInfo(aInfo)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    profiler_tracing(mCategory, mInfo, aBacktrace.release(), TRACING_INTERVAL_START);
  }

  ~GeckoProfilerTracingRAII() {
    profiler_tracing(mCategory, mInfo, TRACING_INTERVAL_END);
  }

protected:
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  const char* mCategory;
  const char* mInfo;
};

class MOZ_STACK_CLASS SamplerStackFrameRAII {
public:
  
  SamplerStackFrameRAII(const char *aInfo,
    js::ProfileEntry::Category aCategory, uint32_t line)
  {
    mHandle = mozilla_sampler_call_enter(aInfo, aCategory, this, false, line);
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
  
  SamplerStackFramePrintfRAII(const char *aInfo,
    js::ProfileEntry::Category aCategory, uint32_t line, const char *aFormat, ...)
  {
    if (profiler_is_active() && !profiler_in_privacy_mode()) {
      va_list args;
      va_start(args, aFormat);
      char buff[SAMPLER_MAX_STRING];

      
      
#if _MSC_VER
      _vsnprintf(buff, SAMPLER_MAX_STRING, aFormat, args);
      _snprintf(mDest, SAMPLER_MAX_STRING, "%s %s", aInfo, buff);
#else
      ::vsnprintf(buff, SAMPLER_MAX_STRING, aFormat, args);
      ::snprintf(mDest, SAMPLER_MAX_STRING, "%s %s", aInfo, buff);
#endif
      mHandle = mozilla_sampler_call_enter(mDest, aCategory, this, true, line);
      va_end(args);
    } else {
      mHandle = mozilla_sampler_call_enter(aInfo, aCategory, this, false, line);
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
    return nullptr;
  return tlsPseudoStack.get();
}

inline void* mozilla_sampler_call_enter(const char *aInfo,
  js::ProfileEntry::Category aCategory, void *aFrameAddress, bool aCopy, uint32_t line)
{
  
  
  if (!stack_key_initialized)
    return nullptr;

  PseudoStack *stack = tlsPseudoStack.get();
  
  
  
  
  if (!stack) {
    return stack;
  }
  stack->push(aInfo, aCategory, aFrameAddress, aCopy, line);

  
  
  
  
  
  return stack;
}

inline void mozilla_sampler_call_exit(void *aHandle)
{
  if (!aHandle)
    return;

  PseudoStack *stack = (PseudoStack*)aHandle;
  stack->popAndMaybeDelete();
}

void mozilla_sampler_add_marker(const char *aMarker, ProfilerMarkerPayload *aPayload);

static inline
void profiler_log(const char *str)
{
  profiler_tracing("log", str, TRACING_EVENT);
}

static inline
void profiler_log(const char *fmt, va_list args)
{
  mozilla_sampler_log(fmt, args);
}

#endif 
