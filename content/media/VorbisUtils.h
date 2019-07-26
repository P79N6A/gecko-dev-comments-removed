





#ifndef VORBISUTILS_H_
#define VORBISUTILS_H_

#ifdef MOZ_SAMPLE_TYPE_S16
#include <ogg/os_types.h>
typedef ogg_int32_t VorbisPCMValue;

#define MOZ_CLIP_TO_15(x) ((x)<-32768?-32768:(x)<=32767?(x):32767)

#define MOZ_CONVERT_VORBIS_SAMPLE(x) \
 (static_cast<AudioDataValue>(MOZ_CLIP_TO_15((x)>>9)))

#else 

typedef float VorbisPCMValue;

#define MOZ_CONVERT_VORBIS_SAMPLE(x) (x)

#endif

#endif 
