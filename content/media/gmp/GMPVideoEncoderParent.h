




#ifndef GMPVideoEncoderParent_h_
#define GMPVideoEncoderParent_h_

#include "mozilla/RefPtr.h"
#include "gmp-video-encode.h"
#include "mozilla/gmp/PGMPVideoEncoderParent.h"
#include "GMPMessageUtils.h"
#include "GMPSharedMemManager.h"
#include "GMPVideoHost.h"

namespace mozilla {
namespace gmp {

class GMPParent;

class GMPVideoEncoderParent : public GMPVideoEncoder,
                              public PGMPVideoEncoderParent,
                              public GMPSharedMemManager
{
public:
  NS_INLINE_DECL_REFCOUNTING(GMPVideoEncoderParent)

  GMPVideoEncoderParent(GMPParent *aPlugin);

  GMPVideoHostImpl& Host();

  
  virtual GMPVideoErr InitEncode(const GMPVideoCodec& aCodecSettings,
                                 GMPEncoderCallback* aCallback,
                                 int32_t aNumberOfCores,
                                 uint32_t aMaxPayloadSize) MOZ_OVERRIDE;
  virtual GMPVideoErr Encode(GMPVideoi420Frame* aInputFrame,
                             const GMPCodecSpecificInfo& aCodecSpecificInfo,
                             const std::vector<GMPVideoFrameType>& aFrameTypes) MOZ_OVERRIDE;
  virtual GMPVideoErr SetChannelParameters(uint32_t aPacketLoss, uint32_t aRTT) MOZ_OVERRIDE;
  virtual GMPVideoErr SetRates(uint32_t aNewBitRate, uint32_t aFrameRate) MOZ_OVERRIDE;
  virtual GMPVideoErr SetPeriodicKeyFrames(bool aEnable) MOZ_OVERRIDE;
  virtual void EncodingComplete() MOZ_OVERRIDE;

  
  virtual void CheckThread();
  virtual bool Alloc(size_t aSize, Shmem::SharedMemory::SharedMemoryType aType, Shmem* aMem)
  {
#ifdef GMP_SAFE_SHMEM
    return AllocShmem(aSize, aType, aMem);
#else
    return AllocUnsafeShmem(aSize, aType, aMem);
#endif
  }
  virtual void Dealloc(Shmem& aMem)
  {
    DeallocShmem(aMem);
  }

private:
  virtual ~GMPVideoEncoderParent();

  
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual bool RecvEncoded(const GMPVideoEncodedFrameData& aEncodedFrame,
                           const GMPCodecSpecificInfo& aCodecSpecificInfo) MOZ_OVERRIDE;
  virtual bool RecvParentShmemForPool(Shmem& aFrameBuffer) MOZ_OVERRIDE;
  virtual bool AnswerNeedShmem(const uint32_t& aEncodedBufferSize,
                               Shmem* aMem) MOZ_OVERRIDE;
  virtual bool Recv__delete__() MOZ_OVERRIDE;

  bool mCanSendMessages;
  GMPParent* mPlugin;
  GMPEncoderCallback* mCallback;
  GMPVideoHostImpl mVideoHost;
};

} 
} 

#endif 
