




#ifndef PROFILER_FUNCS_H
#define PROFILER_FUNCS_H

#include "mozilla/NullPtr.h"
#include <stdint.h>

namespace mozilla {
class TimeDuration;
class TimeStamp;
}

using mozilla::TimeStamp;
using mozilla::TimeDuration;

struct JSContext;
class JSObject;



inline void* mozilla_sampler_call_enter(const char *aInfo, void *aFrameAddress = NULL,
                                        bool aCopy = false, uint32_t line = 0);
inline void  mozilla_sampler_call_exit(void* handle);
inline void  mozilla_sampler_add_marker(const char *aInfo);

void mozilla_sampler_start(int aEntries, double aInterval,
                           const char** aFeatures, uint32_t aFeatureCount,
                           const char** aThreadNameFilters, uint32_t aFilterCount);

void mozilla_sampler_stop();

bool mozilla_sampler_is_active();

void mozilla_sampler_responsiveness(const TimeStamp& time);

void mozilla_sampler_frame_number(int frameNumber);

const double* mozilla_sampler_get_responsiveness();

void mozilla_sampler_save();

char* mozilla_sampler_get_profile();

JSObject *mozilla_sampler_get_profile_data(JSContext *aCx);

const char** mozilla_sampler_get_features();

void mozilla_sampler_init(void* stackTop);

void mozilla_sampler_shutdown();

void mozilla_sampler_print_location1();
void mozilla_sampler_print_location2();




void mozilla_sampler_lock();


void mozilla_sampler_unlock();


bool mozilla_sampler_register_thread(const char* name, void* stackTop);
void mozilla_sampler_unregister_thread();

double mozilla_sampler_time();


extern bool sps_version2();

#endif

