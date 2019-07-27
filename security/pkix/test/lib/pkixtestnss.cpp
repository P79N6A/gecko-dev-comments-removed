























#include "pkixtestutil.h"

#include <limits>

#include "cryptohi.h"
#include "keyhi.h"
#include "pk11pub.h"
#include "pkix/pkixnss.h"
#include "secerr.h"

namespace mozilla { namespace pkix { namespace test {

namespace {

typedef ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
  ScopedSECKEYPublicKey;
typedef ScopedPtr<SECKEYPrivateKey, SECKEY_DestroyPrivateKey>
  ScopedSECKEYPrivateKey;

class NSSTestKeyPair : public TestKeyPair
{
public:
  
  NSSTestKeyPair(const ByteString& spki,
                 const ByteString& spk,
                 SECKEYPrivateKey* privateKey)
    : TestKeyPair(spki, spk)
    , privateKey(privateKey)
  {
  }

  virtual Result SignData(const ByteString& tbs,
                          SignatureAlgorithm signatureAlgorithm,
                           ByteString& signature) const
  {
    SECOidTag signatureAlgorithmOidTag;
    switch (signatureAlgorithm) {
      case SignatureAlgorithm::rsa_pkcs1_with_sha256:
        signatureAlgorithmOidTag = SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION;
        break;
      default:
        return Result::FATAL_ERROR_INVALID_ARGS;
    }

    SECItem signatureItem;
    if (SEC_SignData(&signatureItem, tbs.data(), tbs.length(),
                     privateKey.get(), signatureAlgorithmOidTag)
          != SECSuccess) {
      return MapPRErrorCodeToResult(PR_GetError());
    }
    signature.assign(signatureItem.data, signatureItem.len);
    SECITEM_FreeItem(&signatureItem, false);
    return Success;
  }

  virtual TestKeyPair* Clone() const
  {
    ScopedSECKEYPrivateKey
      privateKeyCopy(SECKEY_CopyPrivateKey(privateKey.get()));
    if (!privateKeyCopy) {
      return nullptr;
    }
    return new (std::nothrow) NSSTestKeyPair(subjectPublicKeyInfo,
                                             subjectPublicKey,
                                             privateKeyCopy.release());
  }

private:
  ScopedSECKEYPrivateKey privateKey;
};

} 





TestKeyPair* CreateTestKeyPair(const ByteString& spki,
                               const ByteString& spk,
                               SECKEYPrivateKey* privateKey)
{
  return new (std::nothrow) NSSTestKeyPair(spki, spk, privateKey);
}

TestKeyPair*
GenerateKeyPair()
{
  ScopedPtr<PK11SlotInfo, PK11_FreeSlot> slot(PK11_GetInternalSlot());
  if (!slot) {
    return nullptr;
  }

  
  
  
  for (uint32_t retries = 0; retries < 10; retries++) {
    PK11RSAGenParams params;
    params.keySizeInBits = 2048;
    params.pe = 3;
    SECKEYPublicKey* publicKeyTemp = nullptr;
    ScopedSECKEYPrivateKey
      privateKey(PK11_GenerateKeyPair(slot.get(), CKM_RSA_PKCS_KEY_PAIR_GEN,
                                      &params, &publicKeyTemp, false, true,
                                      nullptr));
    ScopedSECKEYPublicKey publicKey(publicKeyTemp);
    if (privateKey) {
      ScopedSECItem
        spkiDER(SECKEY_EncodeDERSubjectPublicKeyInfo(publicKey.get()));
      if (!spkiDER) {
        return nullptr;
      }
      ScopedPtr<CERTSubjectPublicKeyInfo, SECKEY_DestroySubjectPublicKeyInfo>
        spki(SECKEY_CreateSubjectPublicKeyInfo(publicKey.get()));
      if (!spki) {
        return nullptr;
      }
      SECItem spkDER = spki->subjectPublicKey;
      DER_ConvertBitString(&spkDER); 
      return CreateTestKeyPair(ByteString(spkiDER->data, spkiDER->len),
                               ByteString(spkDER.data, spkDER.len),
                               privateKey.release());
    }

    assert(!publicKeyTemp);

    if (PR_GetError() != SEC_ERROR_PKCS11_FUNCTION_FAILED) {
      break;
    }

    
    
    
    static const uint8_t RANDOM_NUMBER[] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    if (PK11_RandomUpdate((void*) &RANDOM_NUMBER,
                          sizeof(RANDOM_NUMBER)) != SECSuccess) {
      break;
    }
  }

  return nullptr;
}

ByteString
SHA1(const ByteString& toHash)
{
  if (toHash.length() >
        static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
    return ENCODING_FAILED;
  }

  uint8_t digestBuf[SHA1_LENGTH];
  SECStatus srv = PK11_HashBuf(SEC_OID_SHA1, digestBuf, toHash.data(),
                               static_cast<int32_t>(toHash.length()));
  if (srv != SECSuccess) {
    return ENCODING_FAILED;
  }
  return ByteString(digestBuf, sizeof(digestBuf));
}

Result
TestCheckPublicKey(Input subjectPublicKeyInfo)
{
  return CheckPublicKey(subjectPublicKeyInfo);
}

Result
TestVerifySignedData(const SignedDataWithSignature& signedData,
                     Input subjectPublicKeyInfo)
{
  return VerifySignedData(signedData, subjectPublicKeyInfo, nullptr);
}

Result
TestDigestBuf(Input item,  uint8_t* digestBuf, size_t digestBufLen)
{
  return DigestBuf(item, digestBuf, digestBufLen);
}

} } } 
