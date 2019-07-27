























#ifndef mozilla_pkix_test__pkixtestutils_h
#define mozilla_pkix_test__pkixtestutils_h

#include <ctime>
#include <stdint.h>

#include "cert.h"
#include "keyhi.h"
#include "pkix/enumclass.h"
#include "pkix/pkixtypes.h"
#include "pkix/ScopedPtr.h"
#include "seccomon.h"

namespace mozilla { namespace pkix { namespace test {














#define MOZILLA_PKIX_ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

class TestInput : public Input
{
public:
  template <size_t N>
  explicit TestInput(const char (&valueString)[N])
    : Input(reinterpret_cast<const uint8_t(&)[N-1]>(valueString))
  {
  }
};

namespace {

inline void
SECITEM_FreeItem_true(SECItem* item)
{
  SECITEM_FreeItem(item, true);
}

} 

typedef ScopedPtr<CERTCertificate, CERT_DestroyCertificate> ScopedCERTCertificate;
typedef ScopedPtr<CERTCertList, CERT_DestroyCertList> ScopedCERTCertList;
typedef mozilla::pkix::ScopedPtr<SECItem, SECITEM_FreeItem_true> ScopedSECItem;
typedef mozilla::pkix::ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
  ScopedSECKEYPublicKey;
typedef mozilla::pkix::ScopedPtr<SECKEYPrivateKey, SECKEY_DestroyPrivateKey>
  ScopedSECKEYPrivateKey;


mozilla::pkix::Time YMDHMS(int16_t year, int16_t month, int16_t day,
                           int16_t hour, int16_t minutes, int16_t seconds);

Result GenerateKeyPair( ScopedSECKEYPublicKey& publicKey,
                        ScopedSECKEYPrivateKey& privateKey);


const SECItem* ASCIIToDERName(PLArenaPool* arena, const char* cn);










Result TamperOnce(SECItem& item, const uint8_t* from, size_t fromLen,
                  const uint8_t* to, size_t toLen);

Result InitInputFromSECItem(const SECItem* secItem,  Input& input);




enum Version { v1 = 0, v2 = 1, v3 = 2 };













SECItem* CreateEncodedCertificate(PLArenaPool* arena, long version,
                                  SECOidTag signature,
                                  const SECItem* serialNumber,
                                  const SECItem* issuerNameDER,
                                  std::time_t notBefore, std::time_t notAfter,
                                  const SECItem* subjectNameDER,
                      SECItem const* const* extensions,
                      SECKEYPrivateKey* issuerPrivateKey,
                                  SECOidTag signatureHashAlg,
                           ScopedSECKEYPrivateKey& privateKey);

SECItem* CreateEncodedSerialNumber(PLArenaPool* arena, long value);

MOZILLA_PKIX_ENUM_CLASS ExtensionCriticality { NotCritical = 0, Critical = 1 };


SECItem* CreateEncodedBasicConstraints(PLArenaPool* arena, bool isCA,
                                        long* pathLenConstraint,
                                       ExtensionCriticality criticality);






SECItem* CreateEncodedEKUExtension(PLArenaPool* arena,
                                   const SECOidTag* ekus, size_t ekusCount,
                                   ExtensionCriticality criticality);




class OCSPResponseExtension
{
public:
  SECItem id;
  bool critical;
  SECItem value;
  OCSPResponseExtension* next;
};

class OCSPResponseContext
{
public:
  OCSPResponseContext(PLArenaPool* arena, const CertID& certID, std::time_t time);

  PLArenaPool* arena;
  const CertID& certID;
  

  

  
  
  
  
  
  
  
  
  
  

  enum OCSPResponseStatus {
    successful = 0,
    malformedRequest = 1,
    internalError = 2,
    tryLater = 3,
    
    sigRequired = 5,
    unauthorized = 6,
  };
  uint8_t responseStatus; 
  bool skipResponseBytes; 

  
  const SECItem* signerNameDER; 
                                
                                

  std::time_t producedAt;

  OCSPResponseExtension* extensions;
  bool includeEmptyExtensions; 
                               
                               
  ScopedSECKEYPrivateKey signerPrivateKey;
  bool badSignature; 
  SECItem const* const* certs; 

  
  
  SECOidTag certIDHashAlg;
  enum CertStatus {
    good = 0,
    revoked = 1,
    unknown = 2,
  };
  uint8_t certStatus; 
  std::time_t revocationTime; 
  std::time_t thisUpdate;
  std::time_t nextUpdate;
  bool includeNextUpdate;
};



SECItem* CreateEncodedOCSPResponse(OCSPResponseContext& context);

} } } 

#endif
