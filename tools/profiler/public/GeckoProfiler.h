















































#ifndef SAMPLER_H
#define SAMPLER_H

#ifndef SPS_STANDALONE
#include "js/TypeDecls.h"
#endif
#include "mozilla/UniquePtr.h"

namespace mozilla {
class TimeStamp;

namespace dom {
class Promise;
} 

} 

enum TracingMetadata {
  TRACING_DEFAULT,
  TRACING_INTERVAL_START,
  TRACING_INTERVAL_END,
  TRACING_EVENT,
  TRACING_EVENT_BACKTRACE,
  TRACING_TIMESTAMP
};

#if !defined(MOZ_ENABLE_PROFILER_SPS) || defined(MOZILLA_XPCOMRT_API)

#include <stdint.h>
#include <stdarg.h>




#define PROFILER_LABEL(name_space, info, category) do {} while (0)



#define PROFILER_LABEL_FUNC(category) do {} while (0)





#define PROFILER_LABEL_PRINTF(name_space, info, category, format, ...) do {} while (0)





#define PROFILER_MARKER(info) do {} while (0)
#define PROFILER_MARKER_PAYLOAD(info, payload) do { nsAutoPtr<ProfilerMarkerPayload> payloadDeletor(payload); } while (0)


#define PROFILER_MAIN_THREAD_LABEL(name_space, info, category) do {} while (0)
#define PROFILER_MAIN_THREAD_LABEL_PRINTF(name_space, info, category, format, ...) do {} while (0)

static inline void profiler_tracing(const char* aCategory, const char* aInfo,
                                    TracingMetadata metaData = TRACING_DEFAULT) {}
class ProfilerBacktrace;

static inline void profiler_tracing(const char* aCategory, const char* aInfo,
                                    ProfilerBacktrace* aCause,
                                    TracingMetadata metaData = TRACING_DEFAULT) {}




static inline void profiler_init(void* stackTop) {};




static inline void profiler_shutdown() {};









static inline void profiler_start(int aProfileEntries, double aInterval,
                              const char** aFeatures, uint32_t aFeatureCount,
                              const char** aThreadNameFilters, uint32_t aFilterCount) {}



static inline void profiler_stop() {}






static inline bool profiler_is_paused() { return false; }
static inline void profiler_pause() {}
static inline void profiler_resume() {}



static inline ProfilerBacktrace* profiler_get_backtrace() { return nullptr; }


static inline void profiler_free_backtrace(ProfilerBacktrace* aBacktrace) {}

static inline bool profiler_is_active() { return false; }




static inline bool profiler_feature_active(const char*) { return false; }


static inline void profiler_responsiveness(const mozilla::TimeStamp& aTime) {}


static inline void profiler_set_frame_number(int frameNumber) {}


static inline mozilla::UniquePtr<char[]> profiler_get_profile(double aSinceTime = 0) {
  return nullptr;
}


static inline JSObject* profiler_get_profile_jsobject(JSContext* aCx,
                                                      double aSinceTime = 0) {
  return nullptr;
}

#ifndef SPS_STANDALONE

static inline void profiler_get_profile_jsobject_async(double aSinceTime = 0,
                                                       mozilla::dom::Promise* = 0) {}
#endif


static inline void profiler_save_profile_to_file(char* aFilename) { }



static inline char** profiler_get_features() { return nullptr; }








static inline void profiler_get_buffer_info(uint32_t *aCurrentPosition,
                                            uint32_t *aTotalSize,
                                            uint32_t *aGeneration)
{
  *aCurrentPosition = 0;
  *aTotalSize = 0;
  *aGeneration = 0;
}




static inline void profiler_lock() {}


static inline void profiler_unlock() {}

static inline void profiler_register_thread(const char* name, void* stackTop) {}
static inline void profiler_unregister_thread() {}




static inline void profiler_sleep_start() {}
static inline void profiler_sleep_end() {}



static inline void profiler_js_operation_callback() {}

static inline double profiler_time() { return 0; }
static inline double profiler_time(const mozilla::TimeStamp& aTime) { return 0; }

static inline bool profiler_in_privacy_mode() { return false; }

static inline void profiler_log(const char *str) {}
static inline void profiler_log(const char *fmt, va_list args) {}

#else

#include "GeckoProfilerImpl.h"

#endif

class GeckoProfilerInitRAII {
public:
  explicit GeckoProfilerInitRAII(void* stackTop) {
    profiler_init(stackTop);
  }
  ~GeckoProfilerInitRAII() {
    profiler_shutdown();
  }
};

class GeckoProfilerSleepRAII {
public:
  GeckoProfilerSleepRAII() {
    profiler_sleep_start();
  }
  ~GeckoProfilerSleepRAII() {
    profiler_sleep_end();
  }
};

#endif 
