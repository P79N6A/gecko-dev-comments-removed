




#include "GMPVideoEncoderChild.h"
#include "GMPChild.h"
#include <stdio.h>
#include "mozilla/unused.h"
#include "GMPVideoEncodedFrameImpl.h"
#include "GMPVideoi420FrameImpl.h"

namespace mozilla {
namespace gmp {

GMPVideoEncoderChild::GMPVideoEncoderChild(GMPChild* aPlugin)
: mPlugin(aPlugin),
  mVideoEncoder(nullptr),
  mVideoHost(MOZ_THIS_IN_INITIALIZER_LIST())
{
  MOZ_ASSERT(mPlugin);
}

GMPVideoEncoderChild::~GMPVideoEncoderChild()
{
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
GMPVideoEncoderChild::CheckThread()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());
}

bool
GMPVideoEncoderChild::RecvInitEncode(const GMPVideoCodec& aCodecSettings,
                                     const nsTArray<uint8_t>& aCodecSpecific,
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
                                 const nsTArray<uint8_t>& aCodecSpecificInfo,
                                 const nsTArray<GMPVideoFrameType>& aFrameTypes)
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
GMPVideoEncoderChild::RecvChildShmemForPool(Shmem& aEncodedBuffer)
{
  if (aEncodedBuffer.IsWritable()) {
    mVideoHost.SharedMemMgr()->MgrDeallocShmem(GMPSharedMemManager::kGMPEncodedData,
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
  if (!mVideoEncoder) {
    return false;
  }

  
  mVideoEncoder->EncodingComplete();

  mVideoHost.DoneWithAPI();

  mPlugin = nullptr;

  unused << Send__delete__(this);

  return true;
}

} 
} 
