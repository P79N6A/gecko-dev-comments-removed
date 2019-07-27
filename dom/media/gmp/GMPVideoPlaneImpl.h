




#ifndef GMPVideoPlaneImpl_h_
#define GMPVideoPlaneImpl_h_

#include "gmp-video-plane.h"
#include "mozilla/ipc/Shmem.h"

namespace mozilla {
namespace gmp {

class GMPVideoHostImpl;
class GMPPlaneData;

class GMPPlaneImpl : public GMPPlane
{
  friend struct IPC::ParamTraits<mozilla::gmp::GMPPlaneImpl>;
public:
  explicit GMPPlaneImpl(GMPVideoHostImpl* aHost);
  GMPPlaneImpl(const GMPPlaneData& aPlaneData, GMPVideoHostImpl* aHost);
  virtual ~GMPPlaneImpl();

  
  
  void DoneWithAPI();
  
  
  
  void ActorDestroyed();

  bool InitPlaneData(GMPPlaneData& aPlaneData);

  
  virtual GMPErr CreateEmptyPlane(int32_t aAllocatedSize,
                                  int32_t aStride,
                                  int32_t aPlaneSize) override;
  virtual GMPErr Copy(const GMPPlane& aPlane) override;
  virtual GMPErr Copy(int32_t aSize,
                      int32_t aStride,
                      const uint8_t* aBuffer) override;
  virtual void Swap(GMPPlane& aPlane) override;
  virtual int32_t AllocatedSize() const override;
  virtual void ResetSize() override;
  virtual bool IsZeroSize() const override;
  virtual int32_t Stride() const override;
  virtual const uint8_t* Buffer() const override;
  virtual uint8_t* Buffer() override;
  virtual void Destroy() override;

private:
  GMPErr MaybeResize(int32_t aNewSize);
  void DestroyBuffer();

  ipc::Shmem mBuffer;
  int32_t mSize;
  int32_t mStride;
  GMPVideoHostImpl* mHost;
};

} 
} 

#endif 
