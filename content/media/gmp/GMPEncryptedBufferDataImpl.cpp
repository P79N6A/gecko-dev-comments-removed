




#include "GMPEncryptedBufferDataImpl.h"
#include "mozilla/gmp/GMPTypes.h"

namespace mozilla {
namespace gmp {

GMPEncryptedBufferDataImpl::GMPEncryptedBufferDataImpl(const CryptoSample& aCrypto)
  : mKeyId(aCrypto.key)
  , mIV(aCrypto.iv)
  , mClearBytes(aCrypto.plain_sizes)
  , mCipherBytes(aCrypto.encrypted_sizes)
{
}

GMPEncryptedBufferDataImpl::GMPEncryptedBufferDataImpl(const GMPDecryptionData& aData)
{
  mKeyId = aData.mKeyId();
  mIV = aData.mIV();
  mClearBytes = aData.mClearBytes();
  mCipherBytes = aData.mCipherBytes();
  MOZ_ASSERT(mClearBytes.Length() == mCipherBytes.Length());
}

void
GMPEncryptedBufferDataImpl::RelinquishData(GMPDecryptionData& aData)
{
  aData.mKeyId() = Move(mKeyId);
  aData.mIV() = Move(mIV);
  aData.mClearBytes() = Move(mClearBytes);
  aData.mCipherBytes() = Move(mCipherBytes);
}

const uint8_t*
GMPEncryptedBufferDataImpl::KeyId() const
{
  return mKeyId.Elements();
}

uint32_t
GMPEncryptedBufferDataImpl::KeyIdSize() const
{
  return mKeyId.Length();
}

const uint8_t*
GMPEncryptedBufferDataImpl::IV() const
{
  return mIV.Elements();
}

uint32_t
GMPEncryptedBufferDataImpl::IVSize() const
{
  return mIV.Length();
}

const uint16_t*
GMPEncryptedBufferDataImpl::ClearBytes() const
{
  return mClearBytes.Elements();
}

const uint32_t*
GMPEncryptedBufferDataImpl::CipherBytes() const
{
  return mCipherBytes.Elements();
}

uint32_t
GMPEncryptedBufferDataImpl::NumSubsamples() const
{
  MOZ_ASSERT(mClearBytes.Length() == mCipherBytes.Length());
  
  
  return std::min<uint32_t>(mClearBytes.Length(), mCipherBytes.Length());
}

} 
} 
