




#ifndef GMPVideoDecoderParent_h_
#define GMPVideoDecoderParent_h_

#include "mozilla/RefPtr.h"
#include "gmp-video-decode.h"
#include "mozilla/gmp/PGMPVideoDecoderParent.h"
#include "GMPMessageUtils.h"
#include "GMPSharedMemManager.h"
#include "GMPVideoHost.h"
#include "GMPVideoDecoderProxy.h"

namespace mozilla {
namespace gmp {

class GMPParent;

class GMPVideoDecoderParent MOZ_FINAL : public PGMPVideoDecoderParent
                                      , public GMPSharedMemManager
                                      , public GMPVideoDecoderProxy
{
public:
  NS_INLINE_DECL_REFCOUNTING(GMPVideoDecoderParent)

  GMPVideoDecoderParent(GMPParent *aPlugin);

  GMPVideoHostImpl& Host();

  
  virtual nsresult InitDecode(const GMPVideoCodec& aCodecSettings,
                              const nsTArray<uint8_t>& aCodecSpecific,
                              GMPVideoDecoderCallback* aCallback,
                              int32_t aCoreCount) MOZ_OVERRIDE;
  virtual nsresult Decode(GMPVideoEncodedFrame* aInputFrame,
                          bool aMissingFrames,
                          const nsTArray<uint8_t>& aCodecSpecificInfo,
                          int64_t aRenderTimeMs = -1) MOZ_OVERRIDE;
  virtual nsresult Reset() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult DecodingComplete() MOZ_OVERRIDE;

  
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
  ~GMPVideoDecoderParent();

  
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
  virtual bool RecvDecoded(const GMPVideoi420FrameData& aDecodedFrame) MOZ_OVERRIDE;
  virtual bool RecvReceivedDecodedReferenceFrame(const uint64_t& aPictureId) MOZ_OVERRIDE;
  virtual bool RecvReceivedDecodedFrame(const uint64_t& aPictureId) MOZ_OVERRIDE;
  virtual bool RecvInputDataExhausted() MOZ_OVERRIDE;
  virtual bool RecvDrainComplete() MOZ_OVERRIDE;
  virtual bool RecvResetComplete() MOZ_OVERRIDE;
  virtual bool RecvParentShmemForPool(Shmem& aEncodedBuffer) MOZ_OVERRIDE;
  virtual bool AnswerNeedShmem(const uint32_t& aFrameBufferSize,
                               Shmem* aMem) MOZ_OVERRIDE;
  virtual bool Recv__delete__() MOZ_OVERRIDE;

  bool mCanSendMessages;
  nsRefPtr<GMPParent> mPlugin;
  GMPVideoDecoderCallback* mCallback;
  GMPVideoHostImpl mVideoHost;
};

} 
} 

#endif 
