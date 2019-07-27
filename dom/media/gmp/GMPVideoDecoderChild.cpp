




#include "GMPVideoDecoderChild.h"
#include "GMPVideoi420FrameImpl.h"
#include "GMPContentChild.h"
#include <stdio.h>
#include "mozilla/unused.h"
#include "GMPVideoEncodedFrameImpl.h"

namespace mozilla {
namespace gmp {

GMPVideoDecoderChild::GMPVideoDecoderChild(GMPContentChild* aPlugin)
: GMPSharedMemManager(aPlugin),
  mPlugin(aPlugin),
  mVideoDecoder(nullptr),
  mVideoHost(this)
{
  MOZ_ASSERT(mPlugin);
}

GMPVideoDecoderChild::~GMPVideoDecoderChild()
{
}

void
GMPVideoDecoderChild::Init(GMPVideoDecoder* aDecoder)
{
  MOZ_ASSERT(aDecoder, "Cannot initialize video decoder child without a video decoder!");
  mVideoDecoder = aDecoder;
}

GMPVideoHostImpl&
GMPVideoDecoderChild::Host()
{
  return mVideoHost;
}

void
GMPVideoDecoderChild::Decoded(GMPVideoi420Frame* aDecodedFrame)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  if (!aDecodedFrame) {
    MOZ_CRASH("Not given a decoded frame!");
  }

  auto df = static_cast<GMPVideoi420FrameImpl*>(aDecodedFrame);

  GMPVideoi420FrameData frameData;
  df->InitFrameData(frameData);
  SendDecoded(frameData);

  aDecodedFrame->Destroy();
}

void
GMPVideoDecoderChild::ReceivedDecodedReferenceFrame(const uint64_t aPictureId)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  SendReceivedDecodedReferenceFrame(aPictureId);
}

void
GMPVideoDecoderChild::ReceivedDecodedFrame(const uint64_t aPictureId)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  SendReceivedDecodedFrame(aPictureId);
}

void
GMPVideoDecoderChild::InputDataExhausted()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  SendInputDataExhausted();
}

void
GMPVideoDecoderChild::DrainComplete()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  SendDrainComplete();
}

void
GMPVideoDecoderChild::ResetComplete()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  SendResetComplete();
}

void
GMPVideoDecoderChild::Error(GMPErr aError)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  SendError(aError);
}

bool
GMPVideoDecoderChild::RecvInitDecode(const GMPVideoCodec& aCodecSettings,
                                     InfallibleTArray<uint8_t>&& aCodecSpecific,
                                     const int32_t& aCoreCount)
{
  if (!mVideoDecoder) {
    return false;
  }

  
  mVideoDecoder->InitDecode(aCodecSettings,
                            aCodecSpecific.Elements(),
                            aCodecSpecific.Length(),
                            this,
                            aCoreCount);
  return true;
}

bool
GMPVideoDecoderChild::RecvDecode(const GMPVideoEncodedFrameData& aInputFrame,
                                 const bool& aMissingFrames,
                                 InfallibleTArray<uint8_t>&& aCodecSpecificInfo,
                                 const int64_t& aRenderTimeMs)
{
  if (!mVideoDecoder) {
    return false;
  }

  auto f = new GMPVideoEncodedFrameImpl(aInputFrame, &mVideoHost);

  
  mVideoDecoder->Decode(f,
                        aMissingFrames,
                        aCodecSpecificInfo.Elements(),
                        aCodecSpecificInfo.Length(),
                        aRenderTimeMs);

  return true;
}

bool
GMPVideoDecoderChild::RecvChildShmemForPool(Shmem&& aFrameBuffer)
{
  if (aFrameBuffer.IsWritable()) {
    mVideoHost.SharedMemMgr()->MgrDeallocShmem(GMPSharedMem::kGMPFrameData,
                                               aFrameBuffer);
  }
  return true;
}

bool
GMPVideoDecoderChild::RecvReset()
{
  if (!mVideoDecoder) {
    return false;
  }

  
  mVideoDecoder->Reset();

  return true;
}

bool
GMPVideoDecoderChild::RecvDrain()
{
  if (!mVideoDecoder) {
    return false;
  }

  
  mVideoDecoder->Drain();

  return true;
}

bool
GMPVideoDecoderChild::RecvDecodingComplete()
{
  if (mVideoDecoder) {
    
    mVideoDecoder->DecodingComplete();
    mVideoDecoder = nullptr;
  }

  mVideoHost.DoneWithAPI();

  mPlugin = nullptr;

  unused << Send__delete__(this);

  return true;
}

} 
} 
