




#ifndef GMPEncryptedBufferDataImpl_h_
#define GMPEncryptedBufferDataImpl_h_

#include "gmp-decryption.h"
#include "mp4_demuxer/DecoderData.h"
#include "nsTArray.h"
#include "mozilla/gmp/GMPTypes.h"

namespace mozilla {
namespace gmp {

class GMPEncryptedBufferDataImpl : public GMPEncryptedBufferMetadata {
private:
  typedef mp4_demuxer::CryptoSample CryptoSample;
public:
  explicit GMPEncryptedBufferDataImpl(const CryptoSample& aCrypto);
  explicit GMPEncryptedBufferDataImpl(const GMPDecryptionData& aData);
  virtual ~GMPEncryptedBufferDataImpl();

  void RelinquishData(GMPDecryptionData& aData);

  virtual const uint8_t* KeyId() const MOZ_OVERRIDE;
  virtual uint32_t KeyIdSize() const MOZ_OVERRIDE;
  virtual const uint8_t* IV() const MOZ_OVERRIDE;
  virtual uint32_t IVSize() const MOZ_OVERRIDE;
  virtual uint32_t NumSubsamples() const MOZ_OVERRIDE;
  virtual const uint16_t* ClearBytes() const MOZ_OVERRIDE;
  virtual const uint32_t* CipherBytes() const MOZ_OVERRIDE;

private:
  nsTArray<uint8_t> mKeyId;
  nsTArray<uint8_t> mIV;
  nsTArray<uint16_t> mClearBytes;
  nsTArray<uint32_t> mCipherBytes;
};

class GMPBufferImpl : public GMPBuffer {
public:
  GMPBufferImpl(uint32_t aId, const nsTArray<uint8_t>& aData)
    : mId(aId)
    , mData(aData)
  {
  }
  virtual uint32_t Id() const {
    return mId;
  }
  virtual uint8_t* Data() {
    return mData.Elements();
  }
  virtual uint32_t Size() const {
    return mData.Length();
  }
  virtual void Resize(uint32_t aSize) {
    mData.SetLength(aSize);
  }

  
  void SetMetadata(GMPEncryptedBufferDataImpl* aMetadata) {
    mMetadata = aMetadata;
  }

  uint32_t mId;
  nsTArray<uint8_t> mData;
  nsAutoPtr<GMPEncryptedBufferDataImpl> mMetadata;
};

} 
} 

#endif 
