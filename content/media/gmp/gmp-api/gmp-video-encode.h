
































#ifndef GMP_VIDEO_ENCODE_h_
#define GMP_VIDEO_ENCODE_h_

#include <vector>
#include <stdint.h>

#include "gmp-video-errors.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"
#include "gmp-video-codec.h"


class GMPEncoderCallback
{
public:
  virtual ~GMPEncoderCallback() {}

  virtual void Encoded(GMPVideoEncodedFrame* aEncodedFrame,
                       const GMPCodecSpecificInfo& aCodecSpecificInfo) = 0;
};


class GMPVideoEncoder
{
public:
  virtual ~GMPVideoEncoder() {}

  
  
  
  
  
  
  
  
  
  virtual GMPVideoErr InitEncode(const GMPVideoCodec& aCodecSettings,
                                 GMPEncoderCallback* aCallback,
                                 int32_t aNumberOfCores,
                                 uint32_t aMaxPayloadSize) = 0;

  
  
  
  
  
  
  
  virtual GMPVideoErr Encode(GMPVideoi420Frame* aInputFrame,
                             const GMPCodecSpecificInfo& aCodecSpecificInfo,
                             const std::vector<GMPVideoFrameType>& aFrameTypes) = 0;

  
  
  
  
  
  
  virtual GMPVideoErr SetChannelParameters(uint32_t aPacketLoss, uint32_t aRTT) = 0;

  
  
  
  
  virtual GMPVideoErr SetRates(uint32_t aNewBitRate, uint32_t aFrameRate) = 0;

  
  
  
  
  virtual GMPVideoErr SetPeriodicKeyFrames(bool aEnable) = 0;

  
  virtual void EncodingComplete() = 0;
};

#endif 
