











































































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
#define SAMPLE_CHECKPOINT(name_space, info)
#define SAMPLE_MARKER(info)


#ifdef ANDROID

#include "sps_sampler.h"

#endif

#endif
