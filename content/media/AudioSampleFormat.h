




#ifndef MOZILLA_AUDIOSAMPLEFORMAT_H_
#define MOZILLA_AUDIOSAMPLEFORMAT_H_

#include "nsAlgorithm.h"
#include <algorithm>

namespace mozilla {








enum AudioSampleFormat
{
  
  AUDIO_FORMAT_S16,
  
  AUDIO_FORMAT_FLOAT32,
  
#ifdef MOZ_SAMPLE_TYPE_S16
  AUDIO_OUTPUT_FORMAT = AUDIO_FORMAT_S16
#else
  AUDIO_OUTPUT_FORMAT = AUDIO_FORMAT_FLOAT32
#endif
};

enum {
  MAX_AUDIO_SAMPLE_SIZE = sizeof(float)
};

template <AudioSampleFormat Format> class AudioSampleTraits;

template <> class AudioSampleTraits<AUDIO_FORMAT_FLOAT32> {
public:
  typedef float Type;
};
template <> class AudioSampleTraits<AUDIO_FORMAT_S16> {
public:
  typedef int16_t Type;
};

typedef AudioSampleTraits<AUDIO_OUTPUT_FORMAT>::Type AudioDataValue;









inline float
AudioSampleToFloat(float aValue)
{
  return aValue;
}
inline float
AudioSampleToFloat(int16_t aValue)
{
  return aValue/32768.0f;
}

template <typename T> T FloatToAudioSample(float aValue);

template <> inline float
FloatToAudioSample<float>(float aValue)
{
  return aValue;
}
template <> inline int16_t
FloatToAudioSample<int16_t>(float aValue)
{
  float v = aValue*32768.0f;
  float clamped = std::max(-32768.0f, std::min(32767.0f, v));
  return int16_t(clamped);
}



template <typename From, typename To> inline void
ConvertAudioSamples(const From* aFrom, To* aTo, int aCount)
{
  for (int i = 0; i < aCount; ++i) {
    aTo[i] = FloatToAudioSample<To>(AudioSampleToFloat(aFrom[i]));
  }
}
inline void
ConvertAudioSamples(const int16_t* aFrom, int16_t* aTo, int aCount)
{
  memcpy(aTo, aFrom, sizeof(*aTo)*aCount);
}
inline void
ConvertAudioSamples(const float* aFrom, float* aTo, int aCount)
{
  memcpy(aTo, aFrom, sizeof(*aTo)*aCount);
}



template <typename From, typename To> inline void
ConvertAudioSamplesWithScale(const From* aFrom, To* aTo, int aCount, float aScale)
{
  if (aScale == 1.0f) {
    ConvertAudioSamples(aFrom, aTo, aCount);
    return;
  }
  for (int i = 0; i < aCount; ++i) {
    aTo[i] = FloatToAudioSample<To>(AudioSampleToFloat(aFrom[i])*aScale);
  }
}
inline void
ConvertAudioSamplesWithScale(const int16_t* aFrom, int16_t* aTo, int aCount, float aScale)
{
  if (aScale == 1.0f) {
    ConvertAudioSamples(aFrom, aTo, aCount);
    return;
  }
  if (0.0f <= aScale && aScale < 1.0f) {
    int32_t scale = int32_t((1 << 16) * aScale);
    for (int i = 0; i < aCount; ++i) {
      aTo[i] = int16_t((int32_t(aFrom[i]) * scale) >> 16);
    }
    return;
  }
  for (int i = 0; i < aCount; ++i) {
    aTo[i] = FloatToAudioSample<int16_t>(AudioSampleToFloat(aFrom[i])*aScale);
  }
}


inline void
ScaleAudioSamples(float* aBuffer, int aCount, float aScale)
{
  for (int32_t i = 0; i < aCount; ++i) {
    aBuffer[i] *= aScale;
  }
}

inline void
ScaleAudioSamples(short* aBuffer, int aCount, float aScale)
{
  int32_t volume = int32_t((1 << 16) * aScale);
  for (int32_t i = 0; i < aCount; ++i) {
    aBuffer[i] = short((int32_t(aBuffer[i]) * volume) >> 16);
  }
}

inline const void*
AddAudioSampleOffset(const void* aBase, AudioSampleFormat aFormat,
                     int32_t aOffset)
{
  static_assert(AUDIO_FORMAT_S16 == 0, "Bad constant");
  static_assert(AUDIO_FORMAT_FLOAT32 == 1, "Bad constant");
  NS_ASSERTION(aFormat == AUDIO_FORMAT_S16 || aFormat == AUDIO_FORMAT_FLOAT32,
               "Unknown format");

  return static_cast<const uint8_t*>(aBase) + (aFormat + 1)*2*aOffset;
}

} 

#endif 
