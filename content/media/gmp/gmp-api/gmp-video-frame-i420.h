
































#ifndef GMP_VIDEO_FRAME_I420_h_
#define GMP_VIDEO_FRAME_I420_h_

#include "gmp-video-errors.h"
#include "gmp-video-frame.h"
#include "gmp-video-plane.h"

#include <stdint.h>

enum GMPPlaneType {
  kGMPYPlane = 0,
  kGMPUPlane = 1,
  kGMPVPlane = 2,
  kGMPNumOfPlanes = 3
};










class GMPVideoi420Frame : public GMPVideoFrame {
public:
  
  
  
  
  
  virtual GMPVideoErr CreateEmptyFrame(int32_t aWidth, int32_t aHeight,
                                       int32_t aStride_y, int32_t aStride_u, int32_t aStride_v) = 0;

  
  
  
  virtual GMPVideoErr CreateFrame(int32_t aSize_y, const uint8_t* aBuffer_y,
                                  int32_t aSize_u, const uint8_t* aBuffer_u,
                                  int32_t aSize_v, const uint8_t* aBuffer_v,
                                  int32_t aWidth, int32_t aHeight,
                                  int32_t aStride_y, int32_t aStride_u, int32_t aStride_v) = 0;

  
  
  
  virtual GMPVideoErr CopyFrame(const GMPVideoi420Frame& aVideoFrame) = 0;

  
  virtual void SwapFrame(GMPVideoi420Frame* aVideoFrame) = 0;

  
  virtual uint8_t* Buffer(GMPPlaneType aType) = 0;

  
  virtual const uint8_t* Buffer(GMPPlaneType aType) const = 0;

  
  virtual int32_t AllocatedSize(GMPPlaneType aType) const = 0;

  
  virtual int32_t Stride(GMPPlaneType aType) const = 0;

  
  virtual GMPVideoErr SetWidth(int32_t aWidth) = 0;

  
  virtual GMPVideoErr SetHeight(int32_t aHeight) = 0;

  
  virtual int32_t Width() const = 0;

  
  virtual int32_t Height() const = 0;

  
  virtual void SetTimestamp(uint32_t aTimestamp) = 0;

  
  virtual uint32_t Timestamp() const = 0;

  
  virtual void SetRenderTime_ms(int64_t aRenderTime_ms) = 0;

  
  virtual int64_t RenderTime_ms() const = 0;

  
  virtual bool IsZeroSize() const = 0;

  
  virtual void ResetSize() = 0;
};

#endif 
