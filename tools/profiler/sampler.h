















































#ifndef SAMPLER_H
#define SAMPLER_H


#ifdef MOZ_ENABLE_PROFILER_SPS

#include "sps_sampler.h"

#else



#define SAMPLER_INIT()
#define SAMPLER_SHUTDOWN()
#define SAMPLER_START(entries, interval, features, featureCount)
#define SAMPLER_STOP()
#define SAMPLER_IS_ACTIVE() false
#define SAMPLER_SAVE()

#define SAMPLER_GET_PROFILE() NULL
#define SAMPLER_GET_PROFILE_DATA(ctx) NULL
#define SAMPLER_RESPONSIVENESS(time) NULL
#define SAMPLER_FRAME_NUMBER(frameNumber)
#define SAMPLER_GET_RESPONSIVENESS() NULL
#define SAMPLER_GET_FEATURES() NULL
#define SAMPLE_LABEL(name_space, info)




#define SAMPLE_LABEL_PRINTF(name_space, info, format, ...)
#define SAMPLE_LABEL_FN(name_space, info)
#define SAMPLE_MARKER(info)
#define SAMPLE_MAIN_THREAD_LABEL_PRINTF(name_space, info, format, ...)
#define SAMPLE_MAIN_THREAD_LABEL_FN(name_space, info)
#define SAMPLE_MAIN_THREAD_MARKER(info)


#define SAMPLER_PRINT_LOCATION()

#endif

#endif 
