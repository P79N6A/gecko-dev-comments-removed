
































#ifndef GMP_VIDEO_ENCODE_h_
#define GMP_VIDEO_ENCODE_h_

#include <vector>
#include <stdint.h>

#include "gmp-errors.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"
#include "gmp-video-codec.h"


class GMPVideoEncoderCallback
{
public:
  virtual ~GMPVideoEncoderCallback() {}

  virtual void Encoded(GMPVideoEncodedFrame* aEncodedFrame,
                       const uint8_t* aCodecSpecificInfo,
                       uint32_t aCodecSpecificInfoLength) = 0;

  
  
  virtual void Error(GMPErr aError) = 0;
};


class GMPVideoEncoder
{
public:
  virtual ~GMPVideoEncoder() {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void InitEncode(const GMPVideoCodec& aCodecSettings,
                          const uint8_t* aCodecSpecific,
                          uint32_t aCodecSpecificLength,
                          GMPVideoEncoderCallback* aCallback,
                          int32_t aNumberOfCores,
                          uint32_t aMaxPayloadSize) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual void Encode(GMPVideoi420Frame* aInputFrame,
                      const uint8_t* aCodecSpecificInfo,
                      uint32_t aCodecSpecificInfoLength,
                      const GMPVideoFrameType* aFrameTypes,
                      uint32_t aFrameTypesLength) = 0;

  
  
  
  
  
  
  virtual void SetChannelParameters(uint32_t aPacketLoss, uint32_t aRTT) = 0;

  
  
  
  
  virtual void SetRates(uint32_t aNewBitRate, uint32_t aFrameRate) = 0;

  
  
  
  
  virtual void SetPeriodicKeyFrames(bool aEnable) = 0;

  
  virtual void EncodingComplete() = 0;
};

#endif 
