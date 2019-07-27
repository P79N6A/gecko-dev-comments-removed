




#include "GMPAudioDecoderChild.h"
#include "GMPContentChild.h"
#include "GMPAudioHost.h"
#include "mozilla/unused.h"
#include <stdio.h>

namespace mozilla {
namespace gmp {

GMPAudioDecoderChild::GMPAudioDecoderChild(GMPContentChild* aPlugin)
  : mPlugin(aPlugin)
  , mAudioDecoder(nullptr)
{
  MOZ_ASSERT(mPlugin);
}

GMPAudioDecoderChild::~GMPAudioDecoderChild()
{
}

void
GMPAudioDecoderChild::Init(GMPAudioDecoder* aDecoder)
{
  MOZ_ASSERT(aDecoder, "Cannot initialize Audio decoder child without a Audio decoder!");
  mAudioDecoder = aDecoder;
}

GMPAudioHostImpl&
GMPAudioDecoderChild::Host()
{
  return mAudioHost;
}

void
GMPAudioDecoderChild::Decoded(GMPAudioSamples* aDecodedSamples)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  if (!aDecodedSamples) {
    MOZ_CRASH("Not given decoded audio samples!");
  }

  GMPAudioDecodedSampleData samples;
  samples.mData().AppendElements((int16_t*)aDecodedSamples->Buffer(),
                                 aDecodedSamples->Size() / sizeof(int16_t));
  samples.mTimeStamp() = aDecodedSamples->TimeStamp();
  samples.mChannelCount() = aDecodedSamples->Channels();
  samples.mSamplesPerSecond() = aDecodedSamples->Rate();

  unused << SendDecoded(samples);

  aDecodedSamples->Destroy();
}

void
GMPAudioDecoderChild::InputDataExhausted()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  unused << SendInputDataExhausted();
}

void
GMPAudioDecoderChild::DrainComplete()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  unused << SendDrainComplete();
}

void
GMPAudioDecoderChild::ResetComplete()
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  unused << SendResetComplete();
}

void
GMPAudioDecoderChild::Error(GMPErr aError)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());

  unused << SendError(aError);
}

bool
GMPAudioDecoderChild::RecvInitDecode(const GMPAudioCodecData& a)
{
  MOZ_ASSERT(mAudioDecoder);
  if (!mAudioDecoder) {
    return false;
  }

  GMPAudioCodec codec;
  codec.mCodecType = a.mCodecType();
  codec.mChannelCount = a.mChannelCount();
  codec.mBitsPerChannel = a.mBitsPerChannel();
  codec.mSamplesPerSecond = a.mSamplesPerSecond();
  codec.mExtraData = a.mExtraData().Elements();
  codec.mExtraDataLen = a.mExtraData().Length();

  
  mAudioDecoder->InitDecode(codec, this);

  return true;
}

bool
GMPAudioDecoderChild::RecvDecode(const GMPAudioEncodedSampleData& aEncodedSamples)
{
  if (!mAudioDecoder) {
    return false;
  }

  GMPAudioSamples* samples = new GMPAudioSamplesImpl(aEncodedSamples);

  
  mAudioDecoder->Decode(samples);

  return true;
}

bool
GMPAudioDecoderChild::RecvReset()
{
  if (!mAudioDecoder) {
    return false;
  }

  
  mAudioDecoder->Reset();

  return true;
}

bool
GMPAudioDecoderChild::RecvDrain()
{
  if (!mAudioDecoder) {
    return false;
  }

  
  mAudioDecoder->Drain();

  return true;
}

bool
GMPAudioDecoderChild::RecvDecodingComplete()
{
  if (mAudioDecoder) {
    
    mAudioDecoder->DecodingComplete();
    mAudioDecoder = nullptr;
  }

  mPlugin = nullptr;

  unused << Send__delete__(this);

  return true;
}

} 
} 
