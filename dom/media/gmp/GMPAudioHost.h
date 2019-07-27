




#ifndef GMPAudioHost_h_
#define GMPAudioHost_h_

#include "gmp-audio-host.h"
#include "gmp-audio-samples.h"
#include "nsTArray.h"
#include "gmp-decryption.h"
#include "nsAutoPtr.h"
#include "GMPEncryptedBufferDataImpl.h"
#include "mozilla/gmp/GMPTypes.h"

namespace mp4_demuxer {
class MP4Sample;
}

namespace mozilla {
class CryptoSample;

namespace gmp {

class GMPAudioSamplesImpl : public GMPAudioSamples {
public:
  explicit GMPAudioSamplesImpl(GMPAudioFormat aFormat);
  explicit GMPAudioSamplesImpl(const GMPAudioEncodedSampleData& aData);
  GMPAudioSamplesImpl(mp4_demuxer::MP4Sample* aSample,
                      uint32_t aChannels,
                      uint32_t aRate);
  virtual ~GMPAudioSamplesImpl();

  virtual GMPAudioFormat GetFormat() override;
  virtual void Destroy() override;
  virtual GMPErr SetBufferSize(uint32_t aSize) override;
  virtual uint32_t Size() override;
  virtual void SetTimeStamp(uint64_t aTimeStamp) override;
  virtual uint64_t TimeStamp() override;
  virtual const uint8_t* Buffer() const override;
  virtual uint8_t* Buffer() override;
  virtual const GMPEncryptedBufferMetadata* GetDecryptionData() const override;

  void InitCrypto(const CryptoSample& aCrypto);

  void RelinquishData(GMPAudioEncodedSampleData& aData);

  virtual uint32_t Channels() const override;
  virtual void SetChannels(uint32_t aChannels) override;
  virtual uint32_t Rate() const override;
  virtual void SetRate(uint32_t aRate) override;

private:
  GMPAudioFormat mFormat;
  nsTArray<uint8_t> mBuffer;
  int64_t mTimeStamp;
  nsAutoPtr<GMPEncryptedBufferDataImpl> mCrypto;
  uint32_t mChannels;
  uint32_t mRate;
};

class GMPAudioHostImpl : public GMPAudioHost
{
public:
  virtual GMPErr CreateSamples(GMPAudioFormat aFormat,
                               GMPAudioSamples** aSamples) override;
private:
};

} 
} 

#endif
