




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

void mozilla_sampler_start1(int aEntries, int aInterval, const char** aFeatures,
                            uint32_t aFeatureCount);
void mozilla_sampler_start2(int aEntries, int aInterval, const char** aFeatures,
                            uint32_t aFeatureCount);

void mozilla_sampler_stop1();
void mozilla_sampler_stop2();

bool mozilla_sampler_is_active1();
bool mozilla_sampler_is_active2();

void mozilla_sampler_responsiveness1(const TimeStamp& time);
void mozilla_sampler_responsiveness2(const TimeStamp& time);

void mozilla_sampler_frame_number1(int frameNumber);
void mozilla_sampler_frame_number2(int frameNumber);

const double* mozilla_sampler_get_responsiveness1();
const double* mozilla_sampler_get_responsiveness2();

void mozilla_sampler_save1();
void mozilla_sampler_save2();

char* mozilla_sampler_get_profile1();
char* mozilla_sampler_get_profile2();

JSObject *mozilla_sampler_get_profile_data1(JSContext *aCx);
JSObject *mozilla_sampler_get_profile_data2(JSContext *aCx);

const char** mozilla_sampler_get_features1();
const char** mozilla_sampler_get_features2();

void mozilla_sampler_init1();
void mozilla_sampler_init2();

void mozilla_sampler_shutdown1();
void mozilla_sampler_shutdown2();

void mozilla_sampler_print_location1();
void mozilla_sampler_print_location2();




void mozilla_sampler_lock1();
void mozilla_sampler_lock2();


void mozilla_sampler_unlock1();
void mozilla_sampler_unlock2();


extern bool sps_version2();

#endif

