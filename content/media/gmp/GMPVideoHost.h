




#ifndef GMPVideoHost_h_
#define GMPVideoHost_h_

#include "gmp-video-host.h"
#include "gmp-video-plane.h"
#include "gmp-video-frame.h"
#include "gmp-video-host.h"
#include "nsTArray.h"

namespace mozilla {
namespace gmp {

class GMPSharedMemManager;
class GMPPlaneImpl;
class GMPVideoEncodedFrameImpl;

class GMPVideoHostImpl : public GMPVideoHost
{
public:
  explicit GMPVideoHostImpl(GMPSharedMemManager* aSharedMemMgr);
  virtual ~GMPVideoHostImpl();

  
  GMPSharedMemManager* SharedMemMgr();
  void DoneWithAPI();
  void ActorDestroyed();
  void PlaneCreated(GMPPlaneImpl* aPlane);
  void PlaneDestroyed(GMPPlaneImpl* aPlane);
  void EncodedFrameCreated(GMPVideoEncodedFrameImpl* aEncodedFrame);
  void EncodedFrameDestroyed(GMPVideoEncodedFrameImpl* aFrame);

  
  virtual GMPErr CreateFrame(GMPVideoFrameFormat aFormat, GMPVideoFrame** aFrame) MOZ_OVERRIDE;
  virtual GMPErr CreatePlane(GMPPlane** aPlane) MOZ_OVERRIDE;

private:
  
  
  
  GMPSharedMemManager* mSharedMemMgr;

  
  
  
  nsTArray<GMPPlaneImpl*> mPlanes;
  nsTArray<GMPVideoEncodedFrameImpl*> mEncodedFrames;
};

} 
} 

#endif 
