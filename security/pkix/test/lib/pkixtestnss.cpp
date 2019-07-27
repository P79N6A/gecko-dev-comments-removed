























#include "pkixtestutil.h"

#include <limits>

#include "cryptohi.h"
#include "keyhi.h"
#include "nss.h"
#include "pk11pqg.h"
#include "pk11pub.h"
#include "pkix/pkixnss.h"
#include "pkixder.h"
#include "pkixutil.h"
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
  reusedKeyPair.reset(GenerateKeyPairInner());
  return reusedKeyPair ? PR_SUCCESS : PR_FAILURE;
}

class NSSTestKeyPair final : public TestKeyPair
{
public:
  
  NSSTestKeyPair(const TestPublicKeyAlgorithm& publicKeyAlg,
                 const ByteString& spk,
                 SECKEYPrivateKey* privateKey)
    : TestKeyPair(publicKeyAlg, spk)
    , privateKey(privateKey)
  {
  }

  Result SignData(const ByteString& tbs,
                  const TestSignatureAlgorithm& signatureAlgorithm,
                   ByteString& signature) const override
  {
    SECOidTag oidTag;
    if (signatureAlgorithm.publicKeyAlg == RSA_PKCS1()) {
      switch (signatureAlgorithm.digestAlg) {
        case TestDigestAlgorithmID::MD2:
          oidTag = SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION;
          break;
        case TestDigestAlgorithmID::MD5:
          oidTag = SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;
          break;
        case TestDigestAlgorithmID::SHA1:
          oidTag = SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION;
          break;
        case TestDigestAlgorithmID::SHA224:
          oidTag = SEC_OID_PKCS1_SHA224_WITH_RSA_ENCRYPTION;
          break;
        case TestDigestAlgorithmID::SHA256:
          oidTag = SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION;
          break;
        case TestDigestAlgorithmID::SHA384:
          oidTag = SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION;
          break;
        case TestDigestAlgorithmID::SHA512:
          oidTag = SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION;
          break;
        MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
      }
    } else {
      abort();
    }

    SECItem signatureItem;
    if (SEC_SignData(&signatureItem, tbs.data(),
                     static_cast<int>(tbs.length()),
                     privateKey.get(), oidTag) != SECSuccess) {
      return MapPRErrorCodeToResult(PR_GetError());
    }
    signature.assign(signatureItem.data, signatureItem.len);
    SECITEM_FreeItem(&signatureItem, false);
    return Success;
  }

  TestKeyPair* Clone() const override
  {
    ScopedSECKEYPrivateKey
      privateKeyCopy(SECKEY_CopyPrivateKey(privateKey.get()));
    if (!privateKeyCopy) {
      return nullptr;
    }
    return new (std::nothrow) NSSTestKeyPair(publicKeyAlg,
                                             subjectPublicKey,
                                             privateKeyCopy.release());
  }

private:
  ScopedSECKEYPrivateKey privateKey;
};

} 





TestKeyPair* CreateTestKeyPair(const TestPublicKeyAlgorithm publicKeyAlg,
                               const SECKEYPublicKey& publicKey,
                               SECKEYPrivateKey* privateKey)
{
  ScopedPtr<CERTSubjectPublicKeyInfo, SECKEY_DestroySubjectPublicKeyInfo>
    spki(SECKEY_CreateSubjectPublicKeyInfo(&publicKey));
  if (!spki) {
    return nullptr;
  }
  SECItem spkDER = spki->subjectPublicKey;
  DER_ConvertBitString(&spkDER); 
  return new (std::nothrow) NSSTestKeyPair(publicKeyAlg,
                                           ByteString(spkDER.data, spkDER.len),
                                           privateKey);
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
      return CreateTestKeyPair(RSA_PKCS1(), *publicKey, privateKey.release());
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

  ByteString p(DSS_P());
  ByteString q(DSS_Q());
  ByteString g(DSS_G());

  static const PQGParams PARAMS = {
    nullptr,
    { siBuffer,
      const_cast<uint8_t*>(p.data()),
      static_cast<unsigned int>(p.length())
    },
    { siBuffer,
      const_cast<uint8_t*>(q.data()),
      static_cast<unsigned int>(q.length())
    },
    { siBuffer,
      const_cast<uint8_t*>(g.data()),
      static_cast<unsigned int>(g.length())
    }
  };

  SECKEYPublicKey* publicKeyTemp = nullptr;
  ScopedSECKEYPrivateKey
    privateKey(PK11_GenerateKeyPair(slot.get(), CKM_DSA_KEY_PAIR_GEN,
                                    const_cast<PQGParams*>(&PARAMS),
                                    &publicKeyTemp, false, true, nullptr));
  if (!privateKey) {
    return nullptr;
  }
  ScopedSECKEYPublicKey publicKey(publicKeyTemp);
  return CreateTestKeyPair(DSS(), *publicKey, privateKey.release());
}

Result
TestVerifyECDSASignedDigest(const SignedDigest& signedDigest,
                            Input subjectPublicKeyInfo)
{
  InitNSSIfNeeded();
  return VerifyECDSASignedDigestNSS(signedDigest, subjectPublicKeyInfo,
                                    nullptr);
}

Result
TestVerifyRSAPKCS1SignedDigest(const SignedDigest& signedDigest,
                               Input subjectPublicKeyInfo)
{
  InitNSSIfNeeded();
  return VerifyRSAPKCS1SignedDigestNSS(signedDigest, subjectPublicKeyInfo,
                                       nullptr);
}

Result
TestDigestBuf(Input item,
              DigestAlgorithm digestAlg,
               uint8_t* digestBuf,
              size_t digestBufLen)
{
  InitNSSIfNeeded();
  return DigestBufNSS(item, digestAlg, digestBuf, digestBufLen);
}

} } } 
