















































#ifndef SAMPLER_H
#define SAMPLER_H

#include "jsfriendapi.h"
#include "mozilla/NullPtr.h"
#include "mozilla/TimeStamp.h"

#ifndef MOZ_ENABLE_PROFILER_SPS




#define PROFILER_LABEL(name_space, info) do {} while (0)





#define PROFILER_LABEL_PRINTF(name_space, info, format, ...) do {} while (0)





#define PROFILER_MARKER(info) do {} while (0)


#define PROFILER_MAIN_THREAD_LABEL(name_space, info) do {} while (0)
#define PROFILER_MAIN_THREAD_LABEL_PRINTF(name_space, info, format, ...) do {} while (0)




static inline void profiler_init() {};




static inline void profiler_shutdown() {};









static inline void profiler_start(int aProfileEntries, int aInterval,
                              const char** aFeatures, uint32_t aFeatureCount) {}



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

static inline void profiler_register_thread(const char* name) {}
static inline void profiler_unregister_thread() {}

#else

#include "GeckoProfilerImpl.h"

#endif

#endif 
