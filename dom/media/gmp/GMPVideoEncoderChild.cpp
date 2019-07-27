




#include "GMPVideoEncoderChild.h"
#include "GMPContentChild.h"
#include <stdio.h>
#include "mozilla/unused.h"
#include "GMPVideoEncodedFrameImpl.h"
#include "GMPVideoi420FrameImpl.h"
#include "runnable_utils.h"

namespace mozilla {
namespace gmp {

GMPVideoEncoderChild::GMPVideoEncoderChild(GMPContentChild* aPlugin)
  : GMPSharedMemManager(aPlugin)
  , mPlugin(aPlugin)
  , mVideoEncoder(nullptr)
  , mVideoHost(this)
  , mNeedShmemIntrCount(0)
  , mPendingEncodeComplete(false)
{
  MOZ_ASSERT(mPlugin);
}

GMPVideoEncoderChild::~GMPVideoEncoderChild()
{
  MOZ_ASSERT(!mNeedShmemIntrCount);
}

void
GMPVideoEncoderChild::Init(GMPVideoEncoder* aEncoder)
{
  MOZ_ASSERT(aEncoder, "Cannot initialize video encoder child without a video encoder!");
  mVideoEncoder = aEncoder;
}

GMPVideoHostImpl&
GMPVideoEncoderChild::Host()
{
  return mVideoHost;
}

void
GMPVideoEncoderChild::Encoded(GMPVideoEncodedFrame* aEncodedFrame,
                              const uint8_t* aCodecSpecificInfo,
                              uint32_t aCodecSpecificInfoLength)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  auto ef = static_cast<GMPVideoEncodedFrameImpl*>(aEncodedFrame);

  GMPVideoEncodedFrameData frameData;
  ef->RelinquishFrameData(frameData);

  nsTArray<uint8_t> codecSpecific;
  codecSpecific.AppendElements(aCodecSpecificInfo, aCodecSpecificInfoLength);
  SendEncoded(frameData, codecSpecific);

  aEncodedFrame->Destroy();
}

void
GMPVideoEncoderChild::Error(GMPErr aError)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  SendError(aError);
}

bool
GMPVideoEncoderChild::RecvInitEncode(const GMPVideoCodec& aCodecSettings,
                                     InfallibleTArray<uint8_t>&& aCodecSpecific,
                                     const int32_t& aNumberOfCores,
                                     const uint32_t& aMaxPayloadSize)
{
  if (!mVideoEncoder) {
    return false;
  }

  
  mVideoEncoder->InitEncode(aCodecSettings,
                            aCodecSpecific.Elements(),
                            aCodecSpecific.Length(),
                            this,
                            aNumberOfCores,
                            aMaxPayloadSize);

  return true;
}

bool
GMPVideoEncoderChild::RecvEncode(const GMPVideoi420FrameData& aInputFrame,
                                 InfallibleTArray<uint8_t>&& aCodecSpecificInfo,
                                 InfallibleTArray<GMPVideoFrameType>&& aFrameTypes)
{
  if (!mVideoEncoder) {
    return false;
  }

  auto f = new GMPVideoi420FrameImpl(aInputFrame, &mVideoHost);

  
  mVideoEncoder->Encode(f,
                        aCodecSpecificInfo.Elements(),
                        aCodecSpecificInfo.Length(),
                        aFrameTypes.Elements(),
                        aFrameTypes.Length());

  return true;
}

bool
GMPVideoEncoderChild::RecvChildShmemForPool(Shmem&& aEncodedBuffer)
{
  if (aEncodedBuffer.IsWritable()) {
    mVideoHost.SharedMemMgr()->MgrDeallocShmem(GMPSharedMem::kGMPEncodedData,
                                               aEncodedBuffer);
  }
  return true;
}

bool
GMPVideoEncoderChild::RecvSetChannelParameters(const uint32_t& aPacketLoss,
                                               const uint32_t& aRTT)
{
  if (!mVideoEncoder) {
    return false;
  }

  
  mVideoEncoder->SetChannelParameters(aPacketLoss, aRTT);

  return true;
}

bool
GMPVideoEncoderChild::RecvSetRates(const uint32_t& aNewBitRate,
                                   const uint32_t& aFrameRate)
{
  if (!mVideoEncoder) {
    return false;
  }

  
  mVideoEncoder->SetRates(aNewBitRate, aFrameRate);

  return true;
}

bool
GMPVideoEncoderChild::RecvSetPeriodicKeyFrames(const bool& aEnable)
{
  if (!mVideoEncoder) {
    return false;
  }

  
  mVideoEncoder->SetPeriodicKeyFrames(aEnable);

  return true;
}

bool
GMPVideoEncoderChild::RecvEncodingComplete()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  if (mNeedShmemIntrCount) {
    
    
    
    
    mPendingEncodeComplete = true;
    return true;
  }

  if (!mVideoEncoder) {
    unused << Send__delete__(this);
    return false;
  }

  
  mVideoEncoder->EncodingComplete();

  mVideoHost.DoneWithAPI();

  mPlugin = nullptr;

  unused << Send__delete__(this);

  return true;
}

bool
GMPVideoEncoderChild::Alloc(size_t aSize,
                            Shmem::SharedMemory::SharedMemoryType aType,
                            Shmem* aMem)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  bool rv;
#ifndef SHMEM_ALLOC_IN_CHILD
  ++mNeedShmemIntrCount;
  rv = CallNeedShmem(aSize, aMem);
  --mNeedShmemIntrCount;
  if (mPendingEncodeComplete) {
    auto t = NewRunnableMethod(this, &GMPVideoEncoderChild::RecvEncodingComplete);
    mPlugin->GMPMessageLoop()->PostTask(FROM_HERE, t);
  }
#else
#ifdef GMP_SAFE_SHMEM
  rv = AllocShmem(aSize, aType, aMem);
#else
  rv = AllocUnsafeShmem(aSize, aType, aMem);
#endif
#endif
  return rv;
}

void
GMPVideoEncoderChild::Dealloc(Shmem& aMem)
{
#ifndef SHMEM_ALLOC_IN_CHILD
  SendParentShmemForPool(aMem);
#else
  DeallocShmem(aMem);
#endif
}

} 
} 
