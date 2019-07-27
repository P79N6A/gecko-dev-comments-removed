




#ifndef PROFILER_FUNCS_H
#define PROFILER_FUNCS_H

#include "js/TypeDecls.h"
#include "js/ProfilingStack.h"
#include <stdint.h>

namespace mozilla {
class TimeStamp;
}

class ProfilerBacktrace;
class ProfilerMarkerPayload;



inline void* mozilla_sampler_call_enter(const char *aInfo, js::ProfileEntry::Category aCategory,
                                        void *aFrameAddress = nullptr, bool aCopy = false,
                                        uint32_t line = 0);

inline void  mozilla_sampler_call_exit(void* handle);

void  mozilla_sampler_add_marker(const char *aInfo,
                                 ProfilerMarkerPayload *aPayload = nullptr);

void mozilla_sampler_start(int aEntries, double aInterval,
                           const char** aFeatures, uint32_t aFeatureCount,
                           const char** aThreadNameFilters, uint32_t aFilterCount);

void mozilla_sampler_stop();

bool mozilla_sampler_is_paused();
void mozilla_sampler_pause();
void mozilla_sampler_resume();

ProfilerBacktrace* mozilla_sampler_get_backtrace();
void mozilla_sampler_free_backtrace(ProfilerBacktrace* aBacktrace);

bool mozilla_sampler_is_active();

bool mozilla_sampler_feature_active(const char* aName);

void mozilla_sampler_responsiveness(const mozilla::TimeStamp& time);

void mozilla_sampler_frame_number(int frameNumber);

const double* mozilla_sampler_get_responsiveness();

void mozilla_sampler_save();

char* mozilla_sampler_get_profile(float aSinceTime);

JSObject *mozilla_sampler_get_profile_data(JSContext *aCx, float aSinceTime);



extern "C" {
  void mozilla_sampler_save_profile_to_file(const char* aFilename);
}

const char** mozilla_sampler_get_features();

void mozilla_sampler_get_buffer_info(uint32_t *aCurrentPosition, uint32_t *aTotalSize,
                                     uint32_t *aGeneration);

void mozilla_sampler_init(void* stackTop);

void mozilla_sampler_shutdown();




void mozilla_sampler_lock();


void mozilla_sampler_unlock();


bool mozilla_sampler_register_thread(const char* name, void* stackTop);
void mozilla_sampler_unregister_thread();

void mozilla_sampler_sleep_start();
void mozilla_sampler_sleep_end();

double mozilla_sampler_time();
double mozilla_sampler_time(const mozilla::TimeStamp& aTime);

void mozilla_sampler_tracing(const char* aCategory, const char* aInfo,
                             TracingMetadata aMetaData);

void mozilla_sampler_tracing(const char* aCategory, const char* aInfo,
                             ProfilerBacktrace* aCause,
                             TracingMetadata aMetaData);

void mozilla_sampler_log(const char *fmt, va_list args);

#endif

