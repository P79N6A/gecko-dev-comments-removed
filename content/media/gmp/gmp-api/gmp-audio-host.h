















#ifndef GMP_AUDIO_HOST_h_
#define GMP_AUDIO_HOST_h_

#include "gmp-errors.h"
#include "gmp-audio-samples.h"

class GMPAudioHost
{
public:
  
  
  virtual GMPErr CreateSamples(GMPAudioFormat aFormat,
                               GMPAudioSamples** aSamples) = 0;
};

#endif 
