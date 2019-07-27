




#include "GMPVideoDecoderParent.h"
#include "GMPVideoEncodedFrameImpl.h"
#include "GMPVideoi420FrameImpl.h"
#include "GMPParent.h"
#include <stdio.h>
#include "mozilla/unused.h"
#include "GMPMessageUtils.h"
#include "nsAutoRef.h"
#include "nsThreadUtils.h"
#include "mozilla/gmp/GMPTypes.h"

template <>
class nsAutoRefTraits<GMPVideoEncodedFrame> : public nsPointerRefTraits<GMPVideoEncodedFrame>
{
public:
  static void Release(GMPVideoEncodedFrame* aFrame) { aFrame->Destroy(); }
};

namespace mozilla {
namespace gmp {











GMPVideoDecoderParent::GMPVideoDecoderParent(GMPParent* aPlugin)
  : mIsOpen(false)
  , mPlugin(aPlugin)
  , mCallback(nullptr)
  , mVideoHost(MOZ_THIS_IN_INITIALIZER_LIST())
{
  MOZ_ASSERT(mPlugin);
}

GMPVideoDecoderParent::~GMPVideoDecoderParent()
{
}

GMPVideoHostImpl&
GMPVideoDecoderParent::Host()
{
  return mVideoHost;
}


void
GMPVideoDecoderParent::Close()
{
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());
  
  
  mCallback = nullptr;
  

  
  nsRefPtr<GMPVideoDecoderParent> kungfudeathgrip(this);
  NS_RELEASE(kungfudeathgrip);
  Shutdown();
}

nsresult
GMPVideoDecoderParent::InitDecode(const GMPVideoCodec& aCodecSettings,
                                  const nsTArray<uint8_t>& aCodecSpecific,
                                  GMPVideoDecoderCallbackProxy* aCallback,
                                  int32_t aCoreCount)
{
  if (mIsOpen) {
    NS_WARNING("Trying to re-init an in-use GMP video decoder!");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!aCallback) {
    return NS_ERROR_FAILURE;
  }
  mCallback = aCallback;

  if (!SendInitDecode(aCodecSettings, aCodecSpecific, aCoreCount)) {
    return NS_ERROR_FAILURE;
  }
  mIsOpen = true;

  
  return NS_OK;
}

nsresult
GMPVideoDecoderParent::Decode(GMPVideoEncodedFrame* aInputFrame,
                              bool aMissingFrames,
                              const nsTArray<uint8_t>& aCodecSpecificInfo,
                              int64_t aRenderTimeMs)
{
  nsAutoRef<GMPVideoEncodedFrame> autoDestroy(aInputFrame);

  if (!mIsOpen) {
    NS_WARNING("Trying to use an dead GMP video decoder");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  auto inputFrameImpl = static_cast<GMPVideoEncodedFrameImpl*>(aInputFrame);

  
  
  
  if (NumInUse(kGMPFrameData) > 3*GMPSharedMemManager::kGMPBufLimit ||
      NumInUse(kGMPEncodedData) > GMPSharedMemManager::kGMPBufLimit) {
    return NS_ERROR_FAILURE;
  }

  GMPVideoEncodedFrameData frameData;
  inputFrameImpl->RelinquishFrameData(frameData);

  if (!SendDecode(frameData,
                  aMissingFrames,
                  aCodecSpecificInfo,
                  aRenderTimeMs)) {
    return NS_ERROR_FAILURE;
  }

  
  return NS_OK;
}

nsresult
GMPVideoDecoderParent::Reset()
{
  if (!mIsOpen) {
    NS_WARNING("Trying to use an dead GMP video decoder");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!SendReset()) {
    return NS_ERROR_FAILURE;
  }

  
  return NS_OK;
}

nsresult
GMPVideoDecoderParent::Drain()
{
  if (!mIsOpen) {
    NS_WARNING("Trying to use an dead GMP video decoder");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!SendDrain()) {
    return NS_ERROR_FAILURE;
  }

  
  return NS_OK;
}


nsresult
GMPVideoDecoderParent::Shutdown()
{
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  
  if (mCallback) {
    mCallback->Terminated();
    mCallback = nullptr;
  }
  mVideoHost.DoneWithAPI();

  if (mIsOpen) {
    
    mIsOpen = false;
    unused << SendDecodingComplete();
  }

  return NS_OK;
}


void
GMPVideoDecoderParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mIsOpen = false;
  if (mCallback) {
    
    mCallback->Terminated();
    mCallback = nullptr;
  }
  if (mPlugin) {
    
    mPlugin->VideoDecoderDestroyed(this);
    mPlugin = nullptr;
  }
  mVideoHost.ActorDestroyed();
}

void
GMPVideoDecoderParent::CheckThread()
{
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());
}

bool
GMPVideoDecoderParent::RecvDecoded(const GMPVideoi420FrameData& aDecodedFrame)
{
  if (!mCallback) {
    return false;
  }

  auto f = new GMPVideoi420FrameImpl(aDecodedFrame, &mVideoHost);

  
  mCallback->Decoded(f);

  return true;
}

bool
GMPVideoDecoderParent::RecvReceivedDecodedReferenceFrame(const uint64_t& aPictureId)
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->ReceivedDecodedReferenceFrame(aPictureId);

  return true;
}

bool
GMPVideoDecoderParent::RecvReceivedDecodedFrame(const uint64_t& aPictureId)
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->ReceivedDecodedFrame(aPictureId);

  return true;
}

bool
GMPVideoDecoderParent::RecvInputDataExhausted()
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->InputDataExhausted();

  return true;
}

bool
GMPVideoDecoderParent::RecvDrainComplete()
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->DrainComplete();

  return true;
}

bool
GMPVideoDecoderParent::RecvResetComplete()
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->ResetComplete();

  return true;
}

bool
GMPVideoDecoderParent::RecvError(const GMPErr& aError)
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->Error(aError);

  return true;
}

bool
GMPVideoDecoderParent::RecvParentShmemForPool(Shmem& aEncodedBuffer)
{
  if (aEncodedBuffer.IsWritable()) {
    mVideoHost.SharedMemMgr()->MgrDeallocShmem(GMPSharedMemManager::kGMPEncodedData,
                                               aEncodedBuffer);
  }
  return true;
}

bool
GMPVideoDecoderParent::AnswerNeedShmem(const uint32_t& aFrameBufferSize,
                                       Shmem* aMem)
{
  ipc::Shmem mem;

  if (!mVideoHost.SharedMemMgr()->MgrAllocShmem(GMPSharedMemManager::kGMPFrameData,
                                                aFrameBufferSize,
                                                ipc::SharedMemory::TYPE_BASIC, &mem))
  {
    return false;
  }
  *aMem = mem;
  mem = ipc::Shmem();
  return true;
}

bool
GMPVideoDecoderParent::Recv__delete__()
{
  if (mPlugin) {
    
    mPlugin->VideoDecoderDestroyed(this);
    mPlugin = nullptr;
  }

  return true;
}

} 
} 
