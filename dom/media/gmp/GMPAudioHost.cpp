




#include "GMPAudioHost.h"
#include "gmp-audio-samples.h"
#include "gmp-errors.h"
#include "GMPEncryptedBufferDataImpl.h"
#include "mp4_demuxer/DecoderData.h"

namespace mozilla {
namespace gmp {

GMPAudioSamplesImpl::GMPAudioSamplesImpl(GMPAudioFormat aFormat)
  : mFormat(aFormat)
  , mTimeStamp(0)
  , mChannels(0)
  , mRate(0)
{
}

GMPAudioSamplesImpl::GMPAudioSamplesImpl(const GMPAudioEncodedSampleData& aData)
  : mFormat(kGMPAudioEncodedSamples)
  , mBuffer(aData.mData())
  , mTimeStamp(aData.mTimeStamp())
  , mChannels(aData.mChannelCount())
  , mRate(aData.mSamplesPerSecond())
{
  if (aData.mDecryptionData().mKeyId().Length() > 0) {
    mCrypto = new GMPEncryptedBufferDataImpl(aData.mDecryptionData());
  }
}

GMPAudioSamplesImpl::GMPAudioSamplesImpl(mp4_demuxer::MP4Sample* aSample,
                                         uint32_t aChannels,
                                         uint32_t aRate)
 : mFormat(kGMPAudioEncodedSamples)
 , mTimeStamp(aSample->composition_timestamp)
 , mChannels(aChannels)
 , mRate(aRate)
{
  mBuffer.AppendElements(aSample->data, aSample->size);
  if (aSample->crypto.valid) {
    mCrypto = new GMPEncryptedBufferDataImpl(aSample->crypto);
  }
}

GMPAudioSamplesImpl::~GMPAudioSamplesImpl()
{
}

GMPAudioFormat
GMPAudioSamplesImpl::GetFormat()
{
  return mFormat;
}

void
GMPAudioSamplesImpl::Destroy()
{
  delete this;
}

GMPErr
GMPAudioSamplesImpl::SetBufferSize(uint32_t aSize)
{
  mBuffer.SetLength(aSize);
  return GMPNoErr;
}

uint32_t
GMPAudioSamplesImpl::Size()
{
  return mBuffer.Length();
}

void
GMPAudioSamplesImpl::SetTimeStamp(uint64_t aTimeStamp)
{
  mTimeStamp = aTimeStamp;
}

uint64_t
GMPAudioSamplesImpl::TimeStamp()
{
  return mTimeStamp;
}

const uint8_t*
GMPAudioSamplesImpl::Buffer() const
{
  return mBuffer.Elements();
}

uint8_t*
GMPAudioSamplesImpl::Buffer()
{
  return mBuffer.Elements();
}

const GMPEncryptedBufferMetadata*
GMPAudioSamplesImpl::GetDecryptionData() const
{
  return mCrypto;
}

void
GMPAudioSamplesImpl::InitCrypto(const CryptoSample& aCrypto)
{
  if (!aCrypto.valid) {
    return;
  }
  mCrypto = new GMPEncryptedBufferDataImpl(aCrypto);
}

void
GMPAudioSamplesImpl::RelinquishData(GMPAudioEncodedSampleData& aData)
{
  aData.mData() = Move(mBuffer);
  aData.mTimeStamp() = TimeStamp();
  if (mCrypto) {
    mCrypto->RelinquishData(aData.mDecryptionData());
  }
}

uint32_t
GMPAudioSamplesImpl::Channels() const
{
  return mChannels;
}

void
GMPAudioSamplesImpl::SetChannels(uint32_t aChannels)
{
  mChannels = aChannels;
}

uint32_t
GMPAudioSamplesImpl::Rate() const
{
  return mRate;
}

void
GMPAudioSamplesImpl::SetRate(uint32_t aRate)
{
  mRate = aRate;
}


GMPErr
GMPAudioHostImpl::CreateSamples(GMPAudioFormat aFormat,
                                GMPAudioSamples** aSamples)
{

  *aSamples = new GMPAudioSamplesImpl(aFormat);
  return GMPNoErr;
}

} 
} 
