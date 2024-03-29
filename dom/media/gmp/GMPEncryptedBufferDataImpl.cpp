




#include "GMPEncryptedBufferDataImpl.h"
#include "mozilla/gmp/GMPTypes.h"
#include "MediaData.h"

namespace mozilla {
namespace gmp {

GMPEncryptedBufferDataImpl::GMPEncryptedBufferDataImpl(const CryptoSample& aCrypto)
  : mKeyId(aCrypto.mKeyId)
  , mIV(aCrypto.mIV)
  , mClearBytes(aCrypto.mPlainSizes)
  , mCipherBytes(aCrypto.mEncryptedSizes)
  , mSessionIdList(aCrypto.mSessionIds)
{
}

GMPEncryptedBufferDataImpl::GMPEncryptedBufferDataImpl(const GMPDecryptionData& aData)
  : mKeyId(aData.mKeyId())
  , mIV(aData.mIV())
  , mClearBytes(aData.mClearBytes())
  , mCipherBytes(aData.mCipherBytes())
  , mSessionIdList(aData.mSessionIds())
{
  MOZ_ASSERT(mClearBytes.Length() == mCipherBytes.Length());
}

GMPEncryptedBufferDataImpl::~GMPEncryptedBufferDataImpl()
{
}

void
GMPEncryptedBufferDataImpl::RelinquishData(GMPDecryptionData& aData)
{
  aData.mKeyId() = Move(mKeyId);
  aData.mIV() = Move(mIV);
  aData.mClearBytes() = Move(mClearBytes);
  aData.mCipherBytes() = Move(mCipherBytes);
  mSessionIdList.RelinquishData(aData.mSessionIds());
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

const GMPStringList*
GMPEncryptedBufferDataImpl::SessionIds() const
{
  return &mSessionIdList;
}

uint32_t
GMPEncryptedBufferDataImpl::NumSubsamples() const
{
  MOZ_ASSERT(mClearBytes.Length() == mCipherBytes.Length());
  
  
  return std::min<uint32_t>(mClearBytes.Length(), mCipherBytes.Length());
}

GMPStringListImpl::GMPStringListImpl(const nsTArray<nsCString>& aStrings)
  : mStrings(aStrings)
{
}

const uint32_t
GMPStringListImpl::Size() const
{
  return mStrings.Length();
}

void
GMPStringListImpl::StringAt(uint32_t aIndex,
                            const char** aOutString,
                            uint32_t *aOutLength) const
{
  if (NS_WARN_IF(aIndex >= Size())) {
    return;
  }

  *aOutString = mStrings[aIndex].BeginReading();
  *aOutLength = mStrings[aIndex].Length();
}

void
GMPStringListImpl::RelinquishData(nsTArray<nsCString>& aStrings)
{
  aStrings = Move(mStrings);
}

GMPStringListImpl::~GMPStringListImpl()
{
}

} 
} 
