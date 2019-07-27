




#include "GMPVideoDecoderParent.h"
#include "prlog.h"
#include "mozilla/unused.h"
#include "nsAutoRef.h"
#include "nsThreadUtils.h"
#include "GMPUtils.h"
#include "GMPVideoEncodedFrameImpl.h"
#include "GMPVideoi420FrameImpl.h"
#include "GMPContentParent.h"
#include "GMPMessageUtils.h"
#include "mozilla/gmp/GMPTypes.h"

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetGMPLog();

#define LOGD(msg) PR_LOG(GetGMPLog(), PR_LOG_DEBUG, msg)
#define LOG(level, msg) PR_LOG(GetGMPLog(), (level), msg)
#else
#define LOGD(msg)
#define LOG(level, msg)
#endif

namespace gmp {











GMPVideoDecoderParent::GMPVideoDecoderParent(GMPContentParent* aPlugin)
  : GMPSharedMemManager(aPlugin)
  , mIsOpen(false)
  , mShuttingDown(false)
  , mPlugin(aPlugin)
  , mCallback(nullptr)
  , mVideoHost(this)
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
  LOGD(("%s: %p", __FUNCTION__, this));
  MOZ_ASSERT(!mPlugin || mPlugin->GMPThread() == NS_GetCurrentThread());
  
  
  mCallback = nullptr;
  

  
  nsRefPtr<GMPVideoDecoderParent> kungfudeathgrip(this);
  Release();
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
GMPVideoDecoderParent::Decode(GMPUniquePtr<GMPVideoEncodedFrame> aInputFrame,
                              bool aMissingFrames,
                              const nsTArray<uint8_t>& aCodecSpecificInfo,
                              int64_t aRenderTimeMs)
{
  if (!mIsOpen) {
    NS_WARNING("Trying to use an dead GMP video decoder");
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  GMPUniquePtr<GMPVideoEncodedFrameImpl> inputFrameImpl(
    static_cast<GMPVideoEncodedFrameImpl*>(aInputFrame.release()));

  
  
  
  if ((NumInUse(GMPSharedMem::kGMPFrameData) > 3*GMPSharedMem::kGMPBufLimit) ||
      (NumInUse(GMPSharedMem::kGMPEncodedData) > GMPSharedMem::kGMPBufLimit)) {
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

const nsCString&
GMPVideoDecoderParent::GetDisplayName() const
{
  if (!mIsOpen) {
    NS_WARNING("Trying to use an dead GMP video decoder");
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  return mPlugin->GetDisplayName();
}


nsresult
GMPVideoDecoderParent::Shutdown()
{
  LOGD(("%s: %p", __FUNCTION__, this));
  MOZ_ASSERT(!mPlugin || mPlugin->GMPThread() == NS_GetCurrentThread());

  if (mShuttingDown) {
    return NS_OK;
  }
  mShuttingDown = true;

  
  if (mCallback) {
    mCallback->Terminated();
    mCallback = nullptr;
  }

  mIsOpen = false;
  unused << SendDecodingComplete();

  return NS_OK;
}


void
GMPVideoDecoderParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mIsOpen = false;
  mVideoHost.DoneWithAPI();

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

bool
GMPVideoDecoderParent::RecvDecoded(const GMPVideoi420FrameData& aDecodedFrame)
{
  if (!mCallback) {
    return false;
  }

  if (!GMPVideoi420FrameImpl::CheckFrameData(aDecodedFrame)) {
    LOG(PR_LOG_ERROR, ("%s: Decoded frame corrupt, ignoring", __FUNCTION__));
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
GMPVideoDecoderParent::RecvShutdown()
{
  Shutdown();
  return true;
}

bool
GMPVideoDecoderParent::RecvParentShmemForPool(Shmem&& aEncodedBuffer)
{
  if (aEncodedBuffer.IsWritable()) {
    mVideoHost.SharedMemMgr()->MgrDeallocShmem(GMPSharedMem::kGMPEncodedData,
                                               aEncodedBuffer);
  }
  return true;
}

bool
GMPVideoDecoderParent::AnswerNeedShmem(const uint32_t& aFrameBufferSize,
                                       Shmem* aMem)
{
  ipc::Shmem mem;

  if (!mVideoHost.SharedMemMgr()->MgrAllocShmem(GMPSharedMem::kGMPFrameData,
                                                aFrameBufferSize,
                                                ipc::SharedMemory::TYPE_BASIC, &mem))
  {
    LOG(PR_LOG_ERROR, ("%s: Failed to get a shared mem buffer for Child! size %u",
                       __FUNCTION__, aFrameBufferSize));
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
