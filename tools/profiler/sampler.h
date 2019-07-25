











































































#ifndef SAMPLER_H
#define SAMPLER_H


#if defined(ANDROID) || defined(__linux__) || defined(XP_MACOSX) || defined(XP_WIN)

#include "sps_sampler.h"

#else



#define SAMPLER_INIT()
#define SAMPLER_DEINIT()
#define SAMPLER_START(entries, interval, features, featureCount)
#define SAMPLER_STOP()
#define SAMPLER_IS_ACTIVE() false
#define SAMPLER_SAVE()

#define SAMPLER_GET_PROFILE() NULL
#define SAMPLER_RESPONSIVENESS(time) NULL
#define SAMPLER_GET_RESPONSIVENESS() NULL
#define SAMPLER_GET_FEATURES() NULL
#define SAMPLE_LABEL(name_space, info)
#define SAMPLE_LABEL_FN(name_space, info)
#define SAMPLE_MARKER(info)

#endif

#endif 
