




#ifndef PROFILER_FUNCS_H
#define PROFILER_FUNCS_H

#include "mozilla/NullPtr.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/TimeStamp.h"
#include "jsfriendapi.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;



inline void* mozilla_sampler_call_enter(const char *aInfo, void *aFrameAddress = NULL,
                                        bool aCopy = false, uint32_t line = 0);
inline void  mozilla_sampler_call_exit(void* handle);
inline void  mozilla_sampler_add_marker(const char *aInfo);

void mozilla_sampler_start(int aEntries, int aInterval, const char** aFeatures,
                            uint32_t aFeatureCount);

void mozilla_sampler_stop();

bool mozilla_sampler_is_active();

void mozilla_sampler_responsiveness(const TimeStamp& time);

void mozilla_sampler_frame_number(int frameNumber);

const double* mozilla_sampler_get_responsiveness();

void mozilla_sampler_save();

char* mozilla_sampler_get_profile();

JSObject *mozilla_sampler_get_profile_data(JSContext *aCx);

const char** mozilla_sampler_get_features();

void mozilla_sampler_init();

void mozilla_sampler_shutdown();

void mozilla_sampler_print_location1();
void mozilla_sampler_print_location2();




void mozilla_sampler_lock();


void mozilla_sampler_unlock();


extern bool sps_version2();

#endif

