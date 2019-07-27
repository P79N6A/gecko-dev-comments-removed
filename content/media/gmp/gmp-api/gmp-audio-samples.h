















#ifndef GMP_AUDIO_FRAME_h_
#define GMP_AUDIO_FRAME_h_

#include <stdint.h>
#include "gmp-errors.h"
#include "gmp-decryption.h"

enum GMPAudioFormat
{
  kGMPAudioEncodedSamples, 
  kGMPAudioIS16Samples, 
  kGMPAudioSamplesFormatInvalid 
};

class GMPAudioSamples {
public:
  
  virtual GMPAudioFormat GetFormat() = 0;
  virtual void Destroy() = 0;

  
  
  
  virtual GMPErr SetBufferSize(uint32_t aSize) = 0;

  
  virtual uint32_t Size() = 0;

  
  
  virtual void SetTimeStamp(uint64_t aTimeStamp) = 0;
  virtual uint64_t TimeStamp() = 0;
  virtual const uint8_t* Buffer() const = 0;
  virtual uint8_t*       Buffer() = 0;

  
  
  virtual const GMPEncryptedBufferMetadata* GetDecryptionData() const = 0;

  virtual uint32_t Channels() const = 0;
  virtual void SetChannels(uint32_t aChannels) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual uint32_t Rate() const = 0;
  virtual void SetRate(uint32_t aRate) = 0;
};

#endif 
