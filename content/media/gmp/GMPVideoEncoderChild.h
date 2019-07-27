




#ifndef GMPVideoEncoderChild_h_
#define GMPVideoEncoderChild_h_

#include "nsString.h"
#include "mozilla/gmp/PGMPVideoEncoderChild.h"
#include "gmp-video-encode.h"
#include "GMPSharedMemManager.h"
#include "GMPVideoHost.h"

namespace mozilla {
namespace gmp {

class GMPChild;

class GMPVideoEncoderChild : public PGMPVideoEncoderChild,
                             public GMPEncoderCallback,
                             public GMPSharedMemManager
{
public:
  GMPVideoEncoderChild(GMPChild* aPlugin);
  virtual ~GMPVideoEncoderChild();

  void Init(GMPVideoEncoder* aEncoder);
  GMPVideoHostImpl& Host();

  
  virtual void Encoded(GMPVideoEncodedFrame* aEncodedFrame,
                       const GMPCodecSpecificInfo& aCodecSpecificInfo) MOZ_OVERRIDE;

  
  virtual bool MgrAllocShmem(size_t aSize,
                             ipc::Shmem::SharedMemory::SharedMemoryType aType,
                             ipc::Shmem* aMem) MOZ_OVERRIDE;
  virtual bool MgrDeallocShmem(Shmem& aMem) MOZ_OVERRIDE;

private:
  
  virtual bool RecvInitEncode(const GMPVideoCodec& aCodecSettings,
                              const int32_t& aNumberOfCores,
                              const uint32_t& aMaxPayloadSize) MOZ_OVERRIDE;
  virtual bool RecvEncode(const GMPVideoi420FrameData& aInputFrame,
                          const GMPCodecSpecificInfo& aCodecSpecificInfo,
                          const InfallibleTArray<int>& aFrameTypes) MOZ_OVERRIDE;
  virtual bool RecvSetChannelParameters(const uint32_t& aPacketLoss,
                                        const uint32_t& aRTT) MOZ_OVERRIDE;
  virtual bool RecvSetRates(const uint32_t& aNewBitRate,
                            const uint32_t& aFrameRate) MOZ_OVERRIDE;
  virtual bool RecvSetPeriodicKeyFrames(const bool& aEnable) MOZ_OVERRIDE;
  virtual bool RecvEncodingComplete() MOZ_OVERRIDE;

  GMPChild* mPlugin;
  GMPVideoEncoder* mVideoEncoder;
  GMPVideoHostImpl mVideoHost;
};

} 
} 

#endif 
