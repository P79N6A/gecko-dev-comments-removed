























#include "pkixtestutil.h"

#include <limits>

#include "cryptohi.h"
#include "keyhi.h"
#include "nss.h"
#include "pk11pqg.h"
#include "pk11pub.h"
#include "pkix/pkixnss.h"
#include "pkixder.h"
#include "prinit.h"
#include "secerr.h"
#include "secitem.h"

namespace mozilla { namespace pkix { namespace test {

namespace {

typedef ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
  ScopedSECKEYPublicKey;
typedef ScopedPtr<SECKEYPrivateKey, SECKEY_DestroyPrivateKey>
  ScopedSECKEYPrivateKey;

inline void
SECITEM_FreeItem_true(SECItem* item)
{
  SECITEM_FreeItem(item, true);
}

typedef mozilla::pkix::ScopedPtr<SECItem, SECITEM_FreeItem_true> ScopedSECItem;

TestKeyPair* GenerateKeyPairInner();

void
InitNSSIfNeeded()
{
  if (NSS_NoDB_Init(nullptr) != SECSuccess) {
    abort();
  }
}

static ScopedTestKeyPair reusedKeyPair;

PRStatus
InitReusedKeyPair()
{
  InitNSSIfNeeded();
  reusedKeyPair = GenerateKeyPairInner();
  return reusedKeyPair ? PR_SUCCESS : PR_FAILURE;
}

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
                          const ByteString& signatureAlgorithm,
                           ByteString& signature) const
  {
    
    
    
    
    
    
    
    if (signatureAlgorithm.length() > 127 ||
        signatureAlgorithm.length() < 4 ||
        signatureAlgorithm.data()[0] != der::SEQUENCE ||
        signatureAlgorithm.data()[2] != der::OIDTag) {
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    SECAlgorithmID signatureAlgorithmID;
    signatureAlgorithmID.algorithm.data =
      const_cast<unsigned char*>(signatureAlgorithm.data() + 4);
    signatureAlgorithmID.algorithm.len = signatureAlgorithm.length() - 4;
    signatureAlgorithmID.parameters.data = nullptr;
    signatureAlgorithmID.parameters.len = 0;
    SECOidTag signatureAlgorithmOidTag =
      SECOID_GetAlgorithmTag(&signatureAlgorithmID);
    if (signatureAlgorithmOidTag == SEC_OID_UNKNOWN) {
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

namespace {

TestKeyPair*
GenerateKeyPairInner()
{
  ScopedPtr<PK11SlotInfo, PK11_FreeSlot> slot(PK11_GetInternalSlot());
  if (!slot) {
    abort();
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
        break;
      }
      ScopedPtr<CERTSubjectPublicKeyInfo, SECKEY_DestroySubjectPublicKeyInfo>
        spki(SECKEY_CreateSubjectPublicKeyInfo(publicKey.get()));
      if (!spki) {
        break;
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

  abort();
#if defined(_MSC_VER) && (_MSC_VER < 1700)
  
  
  
  
  return nullptr;
#endif
}

} 

TestKeyPair*
GenerateKeyPair()
{
  InitNSSIfNeeded();
  return GenerateKeyPairInner();
}

TestKeyPair*
CloneReusedKeyPair()
{
  static PRCallOnceType initCallOnce;
  if (PR_CallOnce(&initCallOnce, InitReusedKeyPair) != PR_SUCCESS) {
    abort();
  }
  assert(reusedKeyPair);
  return reusedKeyPair->Clone();
}

TestKeyPair*
GenerateDSSKeyPair()
{
  InitNSSIfNeeded();

  ScopedPtr<PK11SlotInfo, PK11_FreeSlot> slot(PK11_GetInternalSlot());
  if (!slot) {
    return nullptr;
  }

  PQGParams* pqgParamsTemp = nullptr;
  PQGVerify* pqgVerify = nullptr;
  if (PK11_PQG_ParamGenV2(2048u, 256u, 256u / 8u, &pqgParamsTemp, &pqgVerify)
        != SECSuccess) {
    return nullptr;
  }
  PK11_PQG_DestroyVerify(pqgVerify);
  ScopedPtr<PQGParams, PK11_PQG_DestroyParams> params(pqgParamsTemp);

  SECKEYPublicKey* publicKeyTemp = nullptr;
  ScopedSECKEYPrivateKey
    privateKey(PK11_GenerateKeyPair(slot.get(), CKM_DSA_KEY_PAIR_GEN,
                                    params.get(), &publicKeyTemp, false, true,
                                    nullptr));
  if (!privateKey) {
    return nullptr;
  }
  ScopedSECKEYPublicKey publicKey(publicKeyTemp);

  ScopedSECItem spkiDER(SECKEY_EncodeDERSubjectPublicKeyInfo(publicKey.get()));
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

ByteString
SHA1(const ByteString& toHash)
{
  InitNSSIfNeeded();

  uint8_t digestBuf[SHA1_LENGTH];
  SECStatus srv = PK11_HashBuf(SEC_OID_SHA1, digestBuf, toHash.data(),
                               static_cast<int32_t>(toHash.length()));
  if (srv != SECSuccess) {
    return ByteString();
  }
  return ByteString(digestBuf, sizeof(digestBuf));
}

Result
TestCheckPublicKey(Input subjectPublicKeyInfo)
{
  InitNSSIfNeeded();
  return CheckPublicKey(subjectPublicKeyInfo, MINIMUM_TEST_KEY_BITS);
}

Result
TestVerifySignedData(const SignedDataWithSignature& signedData,
                     Input subjectPublicKeyInfo)
{
  InitNSSIfNeeded();
  return VerifySignedData(signedData, subjectPublicKeyInfo,
                          MINIMUM_TEST_KEY_BITS, nullptr);
}

Result
TestDigestBuf(Input item,  uint8_t* digestBuf, size_t digestBufLen)
{
  InitNSSIfNeeded();
  return DigestBuf(item, digestBuf, digestBufLen);
}

} } } 
