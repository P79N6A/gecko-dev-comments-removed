




#include "GMPVideoEncoderParent.h"
#include "prlog.h"
#include "GMPVideoi420FrameImpl.h"
#include "GMPVideoEncodedFrameImpl.h"
#include "mozilla/unused.h"
#include "GMPMessageUtils.h"
#include "nsAutoRef.h"
#include "GMPParent.h"
#include "mozilla/gmp/GMPTypes.h"
#include "nsThreadUtils.h"
#include "runnable_utils.h"

template <>
class nsAutoRefTraits<GMPVideoi420Frame> : public nsPointerRefTraits<GMPVideoi420Frame>
{
public:
  static void Release(GMPVideoi420Frame* aFrame) { aFrame->Destroy(); }
};

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

#ifdef __CLASS__
#undef __CLASS__
#endif
#define __CLASS__ "GMPVideoEncoderParent"

namespace gmp {











GMPVideoEncoderParent::GMPVideoEncoderParent(GMPParent *aPlugin)
: GMPSharedMemManager(aPlugin),
  mIsOpen(false),
  mPlugin(aPlugin),
  mCallback(nullptr),
  mVideoHost(MOZ_THIS_IN_INITIALIZER_LIST())
{
  MOZ_ASSERT(mPlugin);

  nsresult rv = NS_NewNamedThread("GMPEncoded", getter_AddRefs(mEncodedThread));
  if (NS_FAILED(rv)) {
    MOZ_CRASH();
  }
}

GMPVideoEncoderParent::~GMPVideoEncoderParent()
{
  if (mEncodedThread) {
    mEncodedThread->Shutdown();
  }
}

GMPVideoHostImpl&
GMPVideoEncoderParent::Host()
{
  return mVideoHost;
}


void
GMPVideoEncoderParent::Close()
{
  LOGD(("%s::%s: %p", __CLASS__, __FUNCTION__, this));
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());
  
  
  mCallback = nullptr;
  

  
  nsRefPtr<GMPVideoEncoderParent> kungfudeathgrip(this);
  NS_RELEASE(kungfudeathgrip);
  Shutdown();
}

GMPErr
GMPVideoEncoderParent::InitEncode(const GMPVideoCodec& aCodecSettings,
                                  const nsTArray<uint8_t>& aCodecSpecific,
                                  GMPVideoEncoderCallbackProxy* aCallback,
                                  int32_t aNumberOfCores,
                                  uint32_t aMaxPayloadSize)
{
  LOGD(("%s::%s: %p", __CLASS__, __FUNCTION__, this));
  if (mIsOpen) {
    NS_WARNING("Trying to re-init an in-use GMP video encoder!");
    return GMPGenericErr;;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  if (!aCallback) {
    return GMPGenericErr;
  }
  mCallback = aCallback;

  if (!SendInitEncode(aCodecSettings, aCodecSpecific, aNumberOfCores, aMaxPayloadSize)) {
    return GMPGenericErr;
  }
  mIsOpen = true;

  
  return GMPNoErr;
}

GMPErr
GMPVideoEncoderParent::Encode(GMPVideoi420Frame* aInputFrame,
                              const nsTArray<uint8_t>& aCodecSpecificInfo,
                              const nsTArray<GMPVideoFrameType>& aFrameTypes)
{
  nsAutoRef<GMPVideoi420Frame> frameRef(aInputFrame);

  if (!mIsOpen) {
    NS_WARNING("Trying to use an dead GMP video encoder");
    return GMPGenericErr;
  }

  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  auto inputFrameImpl = static_cast<GMPVideoi420FrameImpl*>(aInputFrame);

  
  
  
  if ((NumInUse(GMPSharedMem::kGMPFrameData) > 3*GMPSharedMem::kGMPBufLimit) ||
      (NumInUse(GMPSharedMem::kGMPEncodedData) > GMPSharedMem::kGMPBufLimit)) {
    return GMPGenericErr;
  }

  GMPVideoi420FrameData frameData;
  inputFrameImpl->InitFrameData(frameData);

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
  if (!mIsOpen) {
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
  if (!mIsOpen) {
    NS_WARNING("Trying to use an dead GMP video decoder");
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
  if (!mIsOpen) {
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
GMPVideoEncoderParent::Shutdown()
{
  LOGD(("%s::%s: %p", __CLASS__, __FUNCTION__, this));
  MOZ_ASSERT(mPlugin->GMPThread() == NS_GetCurrentThread());

  
  if (mCallback) {
    mCallback->Terminated();
    mCallback = nullptr;
  }
  mVideoHost.DoneWithAPI();
  if (mIsOpen) {
    
    mIsOpen = false;
    unused << SendEncodingComplete();
  }
}


void
GMPVideoEncoderParent::ActorDestroy(ActorDestroyReason aWhy)
{
  LOGD(("%s::%s: %p (%d)", __CLASS__, __FUNCTION__, this, (int) aWhy));
  mIsOpen = false;
  if (mCallback) {
    
    mCallback->Terminated();
    mCallback = nullptr;
  }
  
  
  if (mEncodedThread) {
    mEncodedThread->Shutdown();
    mEncodedThread = nullptr;
  }
  if (mPlugin) {
    
    mPlugin->VideoEncoderDestroyed(this);
    mPlugin = nullptr;
  }
  mVideoHost.ActorDestroyed();
}

static void
EncodedCallback(GMPVideoEncoderCallbackProxy* aCallback,
                GMPVideoEncodedFrame* aEncodedFrame,
                nsTArray<uint8_t>* aCodecSpecificInfo,
                nsCOMPtr<nsIThread> aThread)
{
  aCallback->Encoded(aEncodedFrame, *aCodecSpecificInfo);
  delete aCodecSpecificInfo;
  
  
  aThread->Dispatch(WrapRunnable(aEncodedFrame,
                                &GMPVideoEncodedFrame::Destroy),
                   NS_DISPATCH_NORMAL);
}

bool
GMPVideoEncoderParent::RecvEncoded(const GMPVideoEncodedFrameData& aEncodedFrame,
                                   const nsTArray<uint8_t>& aCodecSpecificInfo)
{
  if (!mCallback) {
    return false;
  }

  auto f = new GMPVideoEncodedFrameImpl(aEncodedFrame, &mVideoHost);
  nsTArray<uint8_t> *codecSpecificInfo = new nsTArray<uint8_t>;
  codecSpecificInfo->AppendElements((uint8_t*)aCodecSpecificInfo.Elements(), aCodecSpecificInfo.Length());
  nsCOMPtr<nsIThread> thread = NS_GetCurrentThread();

  mEncodedThread->Dispatch(WrapRunnableNM(&EncodedCallback,
                                          mCallback, f, codecSpecificInfo, thread),
                           NS_DISPATCH_NORMAL);

  return true;
}

bool
GMPVideoEncoderParent::RecvError(const GMPErr& aError)
{
  if (!mCallback) {
    return false;
  }

  
  mCallback->Error(aError);

  return true;
}

bool
GMPVideoEncoderParent::RecvParentShmemForPool(Shmem& aFrameBuffer)
{
  if (aFrameBuffer.IsWritable()) {
    mVideoHost.SharedMemMgr()->MgrDeallocShmem(GMPSharedMem::kGMPFrameData,
                                               aFrameBuffer);
  }
  return true;
}

bool
GMPVideoEncoderParent::AnswerNeedShmem(const uint32_t& aEncodedBufferSize,
                                       Shmem* aMem)
{
  ipc::Shmem mem;

  if (!mVideoHost.SharedMemMgr()->MgrAllocShmem(GMPSharedMem::kGMPEncodedData,
                                                aEncodedBufferSize,
                                                ipc::SharedMemory::TYPE_BASIC, &mem))
  {
    LOG(PR_LOG_ERROR, ("%s::%s: Failed to get a shared mem buffer for Child! size %u",
                       __CLASS__, __FUNCTION__, aEncodedBufferSize));
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
