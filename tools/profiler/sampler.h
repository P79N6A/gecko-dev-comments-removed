











































































#ifndef SAMPLER_H
#define SAMPLER_H

#if defined(_MSC_VER)
#define FULLFUNCTION __FUNCSIG__
#elif (__GNUC__ >= 4)
#define FULLFUNCTION __PRETTY_FUNCTION__
#else
#define FULLFUNCTION __FUNCTION__
#endif



#define SAMPLER_INIT()
#define SAMPLER_DEINIT()
#define SAMPLER_START(entries, interval)
#define SAMPLER_STOP()
#define SAMPLER_IS_ACTIVE() false
#define SAMPLER_SAVE()

#define SAMPLER_GET_PROFILE() NULL
#define SAMPLER_RESPONSIVENESS(time) NULL
#define SAMPLER_GET_RESPONSIVENESS() NULL
#define SAMPLE_CHECKPOINT(name_space, info)
#define SAMPLE_MARKER(info)


#if defined(ANDROID) || defined(XP_MACOSX)

#include "sps_sampler.h"

#endif

#endif
