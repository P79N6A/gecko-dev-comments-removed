




#ifndef GMPVideoi420FrameImpl_h_
#define GMPVideoi420FrameImpl_h_

#include "gmp-video-frame-i420.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/ipc/Shmem.h"
#include "GMPVideoPlaneImpl.h"

namespace mozilla {
namespace gmp {

class GMPVideoi420FrameData;

class GMPVideoi420FrameImpl : public GMPVideoi420Frame
{
  friend struct IPC::ParamTraits<mozilla::gmp::GMPVideoi420FrameImpl>;
public:
  explicit GMPVideoi420FrameImpl(GMPVideoHostImpl* aHost);
  GMPVideoi420FrameImpl(const GMPVideoi420FrameData& aFrameData, GMPVideoHostImpl* aHost);
  virtual ~GMPVideoi420FrameImpl();

  bool InitFrameData(GMPVideoi420FrameData& aFrameData);
  const GMPPlaneImpl* GetPlane(GMPPlaneType aType) const;
  GMPPlaneImpl* GetPlane(GMPPlaneType aType);

  
  virtual GMPVideoFrameFormat GetFrameFormat() MOZ_OVERRIDE;
  virtual void Destroy() MOZ_OVERRIDE;

  
  virtual GMPErr CreateEmptyFrame(int32_t aWidth,
                                  int32_t aHeight,
                                  int32_t aStride_y,
                                  int32_t aStride_u,
                                  int32_t aStride_v) MOZ_OVERRIDE;
  virtual GMPErr CreateFrame(int32_t aSize_y, const uint8_t* aBuffer_y,
                             int32_t aSize_u, const uint8_t* aBuffer_u,
                             int32_t aSize_v, const uint8_t* aBuffer_v,
                             int32_t aWidth,
                             int32_t aHeight,
                             int32_t aStride_y,
                             int32_t aStride_u,
                             int32_t aStride_v) MOZ_OVERRIDE;
  virtual GMPErr CopyFrame(const GMPVideoi420Frame& aFrame) MOZ_OVERRIDE;
  virtual void SwapFrame(GMPVideoi420Frame* aFrame) MOZ_OVERRIDE;
  virtual uint8_t* Buffer(GMPPlaneType aType) MOZ_OVERRIDE;
  virtual const uint8_t* Buffer(GMPPlaneType aType) const MOZ_OVERRIDE;
  virtual int32_t AllocatedSize(GMPPlaneType aType) const MOZ_OVERRIDE;
  virtual int32_t Stride(GMPPlaneType aType) const MOZ_OVERRIDE;
  virtual GMPErr SetWidth(int32_t aWidth) MOZ_OVERRIDE;
  virtual GMPErr SetHeight(int32_t aHeight) MOZ_OVERRIDE;
  virtual int32_t Width() const MOZ_OVERRIDE;
  virtual int32_t Height() const MOZ_OVERRIDE;
  virtual void SetTimestamp(uint64_t aTimestamp) MOZ_OVERRIDE;
  virtual uint64_t Timestamp() const MOZ_OVERRIDE;
  virtual void SetDuration(uint64_t aDuration) MOZ_OVERRIDE;
  virtual uint64_t Duration() const MOZ_OVERRIDE;
  virtual bool IsZeroSize() const MOZ_OVERRIDE;
  virtual void ResetSize() MOZ_OVERRIDE;

private:
  bool CheckDimensions(int32_t aWidth, int32_t aHeight,
                       int32_t aStride_y, int32_t aStride_u, int32_t aStride_v);

  GMPPlaneImpl mYPlane;
  GMPPlaneImpl mUPlane;
  GMPPlaneImpl mVPlane;
  int32_t mWidth;
  int32_t mHeight;
  uint64_t mTimestamp;
  uint64_t mDuration;
};

} 

template<>
struct DefaultDelete<mozilla::gmp::GMPVideoi420FrameImpl>
{
  void operator()(mozilla::gmp::GMPVideoi420FrameImpl* aFrame) const
  {
    aFrame->Destroy();
  }
};

} 

#endif 
