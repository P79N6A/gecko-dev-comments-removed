




#include "GMPVideoEncoderParent.h"
#include "GMPVideoi420FrameImpl.h"
#include "GMPVideoEncodedFrameImpl.h"
#include <stdio.h>
#include "mozilla/unused.h"
#include "GMPMessageUtils.h"
#include "nsAutoRef.h"
#include "GMPParent.h"
#include "mozilla/gmp/GMPTypes.h"
#include "nsThreadUtils.h"

template <>
class nsAutoRefTraits<GMPVideoi420Frame> : public nsPointerRefTraits<GMPVideoi420Frame>
{
public:
  static void Release(GMPVideoi420Frame* aFrame) { aFrame->Destroy(); }
};

namespace mozilla {
namespace gmp {

GMPVideoEncoderParent::GMPVideoEncoderParent(GMPParent *aPlugin)
: mCanSendMessages(true),
  mPlugin(aPlugin),
  mCallback(nullptr),
  mVideoHost(MOZ_THIS_IN_INITIALIZER_LIST())
{
  MOZ_ASSERT(mPlugin);
}

GMPVideoEncoderParent::~GMPVideoEncoderParent()
{
}

GMPVideoHostImpl&
GMPVideoEncoderParent::Host()
{
  return mVideoHost;
}

GMPErr
GMPVideoEncoderParent::InitEncode(const GMPVideoCodec& aCodecSettings,
                                  const nsTArray<uint8_t>& aCodecSpecific,
                                  GMPVideoEncoderCallbackProxy* aCallback,
                                  int32_t aNumberOfCores,
                                  uint32_t aMaxPayloadSize)
{
  if (!mCanSendMessages) {
    NS_WARNING("Trying to use an invalid GMP video encoder!");
    return GMPGenericErr;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!aCallback) {
    return GMPGenericErr;
  }
  mCallback = aCallback;

  if (!SendInitEncode(aCodecSettings, aCodecSpecific, aNumberOfCores, aMaxPayloadSize)) {
    return GMPGenericErr;
  }

  
  return GMPNoErr;
}

GMPErr
GMPVideoEncoderParent::Encode(GMPVideoi420Frame* aInputFrame,
                              const nsTArray<uint8_t>& aCodecSpecificInfo,
                              const nsTArray<GMPVideoFrameType>& aFrameTypes)
{
  nsAutoRef<GMPVideoi420Frame> frameRef(aInputFrame);

  if (!mCanSendMessages) {
    NS_WARNING("Trying to use an invalid GMP video encoder!");
    return GMPGenericErr;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  auto inputFrameImpl = static_cast<GMPVideoi420FrameImpl*>(aInputFrame);

  GMPVideoi420FrameData frameData;
  inputFrameImpl->InitFrameData(frameData);

  
  
  
  if (NumInUse(kGMPFrameData) > 3*GMPSharedMemManager::kGMPBufLimit ||
      NumInUse(kGMPEncodedData) > GMPSharedMemManager::kGMPBufLimit) {
    return GMPGenericErr;
  }

  if (!SendEncode(frameData,
                  aCodecSpecificInfo,
                  aFrameTypes)) {
    return GMPGenericErr;
  }

  
  return GMPNoErr;
}

GMPErr
GMPVideoEncoderParent::SetChannelParameters(uint32_t aPacketLoss, uint32_t aRTT)
{
  if (!mCanSendMessages) {
    NS_WARNING("Trying to use an invalid GMP video encoder!");
    return GMPGenericErr;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!SendSetChannelParameters(aPacketLoss, aRTT)) {
    return GMPGenericErr;
  }

  
  return GMPNoErr;
}

GMPErr
GMPVideoEncoderParent::SetRates(uint32_t aNewBitRate, uint32_t aFrameRate)
{
  if (!mCanSendMessages) {
    NS_WARNING("Trying to use an invalid GMP video encoder!");
    return GMPGenericErr;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!SendSetRates(aNewBitRate, aFrameRate)) {
    return GMPGenericErr;
  }

  
  return GMPNoErr;
}

GMPErr
GMPVideoEncoderParent::SetPeriodicKeyFrames(bool aEnable)
{
  if (!mCanSendMessages) {
    NS_WARNING("Trying to use an invalid GMP video encoder!");
    return GMPGenericErr;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!SendSetPeriodicKeyFrames(aEnable)) {
    return GMPGenericErr;
  }

  
  return GMPNoErr;
}


void
GMPVideoEncoderParent::EncodingComplete()
{
  if (!mCanSendMessages) {
    NS_WARNING("Trying to use an invalid GMP video encoder!");
    return;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  mCanSendMessages = false;

  mCallback = nullptr;

  mVideoHost.DoneWithAPI();

  unused << SendEncodingComplete();
}


void
GMPVideoEncoderParent::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mPlugin) {
    
    mPlugin->VideoEncoderDestroyed(this);
    mPlugin = nullptr;
  }
  mCanSendMessages = false;
  mCallback = nullptr;
  mVideoHost.ActorDestroyed();
}

void
GMPVideoEncoderParent::CheckThread()
{
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());
}

bool
GMPVideoEncoderParent::RecvEncoded(const GMPVideoEncodedFrameData& aEncodedFrame,
                                   const nsTArray<uint8_t>& aCodecSpecificInfo)
{
  if (!mCallback) {
    return false;
  }

  auto f = new GMPVideoEncodedFrameImpl(aEncodedFrame, &mVideoHost);

  
  mCallback->Encoded(f, aCodecSpecificInfo);

  
  
  return true;
}

bool
GMPVideoEncoderParent::RecvParentShmemForPool(Shmem& aFrameBuffer)
{
  if (aFrameBuffer.IsWritable()) {
    mVideoHost.SharedMemMgr()->MgrDeallocShmem(GMPSharedMemManager::kGMPFrameData,
                                               aFrameBuffer);
  }
  return true;
}

bool
GMPVideoEncoderParent::AnswerNeedShmem(const uint32_t& aEncodedBufferSize,
                                       Shmem* aMem)
{
  ipc::Shmem mem;

  if (!mVideoHost.SharedMemMgr()->MgrAllocShmem(GMPSharedMemManager::kGMPEncodedData,
                                                aEncodedBufferSize,
                                                ipc::SharedMemory::TYPE_BASIC, &mem))
  {
    return false;
  }
  *aMem = mem;
  mem = ipc::Shmem();
  return true;
}

bool
GMPVideoEncoderParent::Recv__delete__()
{
  if (mPlugin) {
    
    mPlugin->VideoEncoderDestroyed(this);
    mPlugin = nullptr;
  }

  return true;
}

} 
} 
