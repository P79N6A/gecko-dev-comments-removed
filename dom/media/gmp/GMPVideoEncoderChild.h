




#ifndef GMPVideoEncoderChild_h_
#define GMPVideoEncoderChild_h_

#include "nsString.h"
#include "mozilla/gmp/PGMPVideoEncoderChild.h"
#include "gmp-video-encode.h"
#include "GMPSharedMemManager.h"
#include "GMPVideoHost.h"

namespace mozilla {
namespace gmp {

class GMPContentChild;

class GMPVideoEncoderChild : public PGMPVideoEncoderChild,
                             public GMPVideoEncoderCallback,
                             public GMPSharedMemManager
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GMPVideoEncoderChild);

  explicit GMPVideoEncoderChild(GMPContentChild* aPlugin);

  void Init(GMPVideoEncoder* aEncoder);
  GMPVideoHostImpl& Host();

  
  virtual void Encoded(GMPVideoEncodedFrame* aEncodedFrame,
                       const uint8_t* aCodecSpecificInfo,
                       uint32_t aCodecSpecificInfoLength) override;
  virtual void Error(GMPErr aError) override;

  
  virtual bool Alloc(size_t aSize, Shmem::SharedMemory::SharedMemoryType aType,
    Shmem* aMem) override;
  virtual void Dealloc(Shmem& aMem) override;

private:
  virtual ~GMPVideoEncoderChild();

  
  virtual bool RecvInitEncode(const GMPVideoCodec& aCodecSettings,
                              InfallibleTArray<uint8_t>&& aCodecSpecific,
                              const int32_t& aNumberOfCores,
                              const uint32_t& aMaxPayloadSize) override;
  virtual bool RecvEncode(const GMPVideoi420FrameData& aInputFrame,
                          InfallibleTArray<uint8_t>&& aCodecSpecificInfo,
                          InfallibleTArray<GMPVideoFrameType>&& aFrameTypes) override;
  virtual bool RecvChildShmemForPool(Shmem&& aEncodedBuffer) override;
  virtual bool RecvSetChannelParameters(const uint32_t& aPacketLoss,
                                        const uint32_t& aRTT) override;
  virtual bool RecvSetRates(const uint32_t& aNewBitRate,
                            const uint32_t& aFrameRate) override;
  virtual bool RecvSetPeriodicKeyFrames(const bool& aEnable) override;
  virtual bool RecvEncodingComplete() override;

  GMPContentChild* mPlugin;
  GMPVideoEncoder* mVideoEncoder;
  GMPVideoHostImpl mVideoHost;

  
  
  int mNeedShmemIntrCount;
  bool mPendingEncodeComplete;
};

} 
} 

#endif 
