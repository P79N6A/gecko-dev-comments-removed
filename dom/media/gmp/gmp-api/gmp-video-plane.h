
































#ifndef GMP_VIDEO_PLANE_h_
#define GMP_VIDEO_PLANE_h_

#include "gmp-errors.h"
#include <stdint.h>










class GMPPlane {
public:
  
  
  
  
  virtual GMPErr CreateEmptyPlane(int32_t aAllocatedSize,
                                  int32_t aStride,
                                  int32_t aPlaneSize) = 0;

  
  
  virtual GMPErr Copy(const GMPPlane& aPlane) = 0;

  
  
  
  virtual GMPErr Copy(int32_t aSize, int32_t aStride, const uint8_t* aBuffer) = 0;

  
  virtual void Swap(GMPPlane& aPlane) = 0;

  
  virtual int32_t AllocatedSize() const = 0;

  
  virtual void ResetSize() = 0;

  
  virtual bool IsZeroSize() const = 0;

  
  virtual int32_t Stride() const = 0;

  
  virtual const uint8_t* Buffer() const = 0;

  
  virtual uint8_t* Buffer() = 0;

  
  
  virtual void Destroy() = 0;
};

#endif 
