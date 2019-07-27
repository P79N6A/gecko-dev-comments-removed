
































#ifndef GMP_VIDEO_DECODE_h_
#define GMP_VIDEO_DECODE_h_

#include "gmp-errors.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"
#include "gmp-video-codec.h"
#include <stdint.h>


class GMPVideoDecoderCallback
{
public:
  virtual ~GMPVideoDecoderCallback() {}

  virtual void Decoded(GMPVideoi420Frame* aDecodedFrame) = 0;

  virtual void ReceivedDecodedReferenceFrame(const uint64_t aPictureId) = 0;

  virtual void ReceivedDecodedFrame(const uint64_t aPictureId) = 0;

  virtual void InputDataExhausted() = 0;

  virtual void DrainComplete() = 0;

  virtual void ResetComplete() = 0;

  
  
  virtual void Error(GMPErr aError) = 0;
};


class GMPVideoDecoder
{
public:
  virtual ~GMPVideoDecoder() {}

  
  
  
  
  
  
  
  virtual void InitDecode(const GMPVideoCodec& aCodecSettings,
                          const uint8_t* aCodecSpecific,
                          uint32_t aCodecSpecificLength,
                          GMPVideoDecoderCallback* aCallback,
                          int32_t aCoreCount) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  virtual void Decode(GMPVideoEncodedFrame* aInputFrame,
                      bool aMissingFrames,
                      const uint8_t* aCodecSpecificInfo,
                      uint32_t aCodecSpecificInfoLength,
                      int64_t aRenderTimeMs = -1) = 0;

  
  
  
  
  virtual void Reset() = 0;

  
  
  
  
  virtual void Drain() = 0;

  
  virtual void DecodingComplete() = 0;
};

#endif 
