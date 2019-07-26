















































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


static inline void profiler_responsinveness(const TimeStamp& aTime) {}


static inline double* profiler_get_responsiveness() { return nullptr; }


static inline void profile_set_frame_number(int frameNumber) {}


static inline char* profiler_get_profile() { return nullptr; }


static inline JSObject* profiler_get_profile_object() { return nullptr; }



static inline char** profiler_get_features() { return nullptr; }




static inline void profiler_print_location() {}




static inline void profiler_lock() {}


static inline void profiler_unlock() {}

#else

#include "sps_sampler.h"

#endif

#endif 
