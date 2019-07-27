
































#ifndef GMP_VIDEO_FRAME_ENCODED_h_
#define GMP_VIDEO_FRAME_ENCODED_h_

#include <stdint.h>
#include "gmp-decryption.h"
#include "gmp-video-frame.h"
#include "gmp-video-codec.h"

enum GMPVideoFrameType
{
  kGMPKeyFrame = 0,
  kGMPDeltaFrame = 1,
  kGMPGoldenFrame = 2,
  kGMPAltRefFrame = 3,
  kGMPSkipFrame = 4
};










class GMPVideoEncodedFrame : public GMPVideoFrame
{
public:
  
  virtual GMPErr CreateEmptyFrame(uint32_t aSize) = 0;
  
  virtual GMPErr CopyFrame(const GMPVideoEncodedFrame& aVideoFrame) = 0;
  virtual void     SetEncodedWidth(uint32_t aEncodedWidth) = 0;
  virtual uint32_t EncodedWidth() = 0;
  virtual void     SetEncodedHeight(uint32_t aEncodedHeight) = 0;
  virtual uint32_t EncodedHeight() = 0;
  
  virtual void     SetTimeStamp(uint64_t aTimeStamp) = 0;
  virtual uint64_t TimeStamp() = 0;
  
  
  
  
  virtual void     SetDuration(uint64_t aDuration) = 0;
  virtual uint64_t Duration() const = 0;
  virtual void     SetFrameType(GMPVideoFrameType aFrameType) = 0;
  virtual GMPVideoFrameType FrameType() = 0;
  virtual void     SetAllocatedSize(uint32_t aNewSize) = 0;
  virtual uint32_t AllocatedSize() = 0;
  virtual void     SetSize(uint32_t aSize) = 0;
  virtual uint32_t Size() = 0;
  virtual void     SetCompleteFrame(bool aCompleteFrame) = 0;
  virtual bool     CompleteFrame() = 0;
  virtual const uint8_t* Buffer() const = 0;
  virtual uint8_t*       Buffer() = 0;
  virtual GMPBufferType  BufferType() const = 0;
  virtual void     SetBufferType(GMPBufferType aBufferType) = 0;

  
  
  virtual const GMPEncryptedBufferMetadata* GetDecryptionData() const = 0;
};

#endif 
