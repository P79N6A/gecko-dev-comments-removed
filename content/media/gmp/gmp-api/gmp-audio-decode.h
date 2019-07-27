















#ifndef GMP_AUDIO_DECODE_h_
#define GMP_AUDIO_DECODE_h_

#include "gmp-errors.h"
#include "gmp-audio-samples.h"
#include "gmp-audio-codec.h"
#include <stdint.h>


class GMPAudioDecoderCallback
{
public:
  virtual ~GMPAudioDecoderCallback() {}

  virtual void Decoded(GMPAudioSamples* aDecodedSamples) = 0;

  virtual void InputDataExhausted() = 0;

  virtual void DrainComplete() = 0;

  virtual void ResetComplete() = 0;
};


class GMPAudioDecoder
{
public:
  virtual ~GMPAudioDecoder() {}

  
  
  
  virtual GMPErr InitDecode(const GMPAudioCodec& aCodecSettings,
                            GMPAudioDecoderCallback* aCallback) = 0;

  
  
  virtual GMPErr Decode(GMPAudioSamples* aEncodedSamples) = 0;

  
  
  
  
  virtual GMPErr Reset() = 0;

  
  
  
  
  virtual GMPErr Drain() = 0;

  
  virtual void DecodingComplete() = 0;
};

#endif 
