















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

  
  
  virtual void Error(GMPErr aError) = 0;
};


class GMPAudioDecoder
{
public:
  virtual ~GMPAudioDecoder() {}

  
  
  
  virtual void InitDecode(const GMPAudioCodec& aCodecSettings,
                          GMPAudioDecoderCallback* aCallback) = 0;

  
  
  virtual void Decode(GMPAudioSamples* aEncodedSamples) = 0;

  
  
  
  
  virtual void Reset() = 0;

  
  
  
  
  virtual void Drain() = 0;

  
  virtual void DecodingComplete() = 0;
};

#endif 
