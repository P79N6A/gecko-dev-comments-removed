















































#ifndef SAMPLER_H
#define SAMPLER_H

#include "mozilla/NullPtr.h"
#include "js/TypeDecls.h"

namespace mozilla {
class TimeStamp;
}

#ifndef MOZ_ENABLE_PROFILER_SPS

#include <stdint.h>




#define PROFILER_LABEL(name_space, info) do {} while (0)





#define PROFILER_LABEL_PRINTF(name_space, info, format, ...) do {} while (0)





#define PROFILER_MARKER(info) do {} while (0)


#define PROFILER_MAIN_THREAD_LABEL(name_space, info) do {} while (0)
#define PROFILER_MAIN_THREAD_LABEL_PRINTF(name_space, info, format, ...) do {} while (0)




static inline void profiler_init(void* stackTop) {};




static inline void profiler_shutdown() {};









static inline void profiler_start(int aProfileEntries, double aInterval,
                              const char** aFeatures, uint32_t aFeatureCount,
                              const char** aThreadNameFilters, uint32_t aFilterCount) {}



static inline void profiler_stop() {}

static inline bool profiler_is_active() { return false; }


static inline void profiler_responsiveness(const mozilla::TimeStamp& aTime) {}


static inline double* profiler_get_responsiveness() { return nullptr; }


static inline void profiler_set_frame_number(int frameNumber) {}


static inline char* profiler_get_profile() { return nullptr; }


static inline JSObject* profiler_get_profile_jsobject(JSContext* aCx) { return nullptr; }



static inline char** profiler_get_features() { return nullptr; }




static inline void profiler_print_location() {}




static inline void profiler_lock() {}


static inline void profiler_unlock() {}

static inline void profiler_register_thread(const char* name, void* stackTop) {}
static inline void profiler_unregister_thread() {}



static inline void profiler_js_operation_callback() {}

static inline double profiler_time() { return 0; }

static inline bool profiler_in_privacy_mode() { return false; }

#else

#include "GeckoProfilerImpl.h"

#endif

class GeckoProfilerInitRAII {
public:
  GeckoProfilerInitRAII(void* stackTop) {
    profiler_init(stackTop);
  }
  ~GeckoProfilerInitRAII() {
    profiler_shutdown();
  }
};

#endif 
