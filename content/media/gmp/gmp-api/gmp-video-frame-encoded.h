
































#ifndef GMP_VIDEO_FRAME_ENCODED_h_
#define GMP_VIDEO_FRAME_ENCODED_h_

#include <stdint.h>

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
  
  virtual GMPVideoErr CreateEmptyFrame(uint32_t aSize) = 0;
  
  virtual GMPVideoErr CopyFrame(const GMPVideoEncodedFrame& aVideoFrame) = 0;
  virtual void     SetEncodedWidth(uint32_t aEncodedWidth) = 0;
  virtual uint32_t EncodedWidth() = 0;
  virtual void     SetEncodedHeight(uint32_t aEncodedHeight) = 0;
  virtual uint32_t EncodedHeight() = 0;
  virtual void     SetTimeStamp(uint32_t aTimeStamp) = 0;
  virtual uint32_t TimeStamp() = 0;
  virtual void     SetCaptureTime(int64_t aCaptureTime) = 0;
  virtual int64_t  CaptureTime() = 0;
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
};

#endif 
