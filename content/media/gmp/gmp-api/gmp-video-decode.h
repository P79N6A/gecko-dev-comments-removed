
































#ifndef GMP_VIDEO_DECODE_h_
#define GMP_VIDEO_DECODE_h_

#include "gmp-video-errors.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"
#include "gmp-video-codec.h"
#include <stdint.h>


class GMPDecoderCallback
{
public:
  virtual ~GMPDecoderCallback() {}

  virtual void Decoded(GMPVideoi420Frame* aDecodedFrame) = 0;

  virtual void ReceivedDecodedReferenceFrame(const uint64_t aPictureId) = 0;

  virtual void ReceivedDecodedFrame(const uint64_t aPictureId) = 0;

  virtual void InputDataExhausted() = 0;
};


class GMPVideoDecoder
{
public:
  virtual ~GMPVideoDecoder() {}

  
  
  virtual GMPVideoErr InitDecode(const GMPVideoCodec& aCodecSettings,
                                 GMPDecoderCallback* aCallback,
                                 int32_t aCoreCount) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual GMPVideoErr Decode(GMPVideoEncodedFrame* aInputFrame,
                             bool aMissingFrames,
                             const GMPCodecSpecificInfo& aCodecSpecificInfo,
                             int64_t aRenderTimeMs = -1) = 0;

  
  virtual GMPVideoErr Reset() = 0;

  
  virtual GMPVideoErr Drain() = 0;

  
  virtual void DecodingComplete() = 0;
};

#endif 
