























#ifndef mozilla_pkix__pkixtypes_h
#define mozilla_pkix__pkixtypes_h

#include "pkix/enumclass.h"
#include "pkix/ScopedPtr.h"
#include "plarena.h"
#include "cert.h"
#include "keyhi.h"
#include "stdint.h"

namespace mozilla { namespace pkix {

inline void
ArenaFalseCleaner(PLArenaPool *arena) {
  
  
  return PORT_FreeArena(arena, PR_FALSE);
}

typedef ScopedPtr<PLArenaPool, ArenaFalseCleaner> ScopedPLArenaPool;

typedef ScopedPtr<CERTCertificate, CERT_DestroyCertificate>
        ScopedCERTCertificate;
typedef ScopedPtr<CERTCertList, CERT_DestroyCertList> ScopedCERTCertList;
typedef ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
        ScopedSECKEYPublicKey;

MOZILLA_PKIX_ENUM_CLASS EndEntityOrCA { MustBeEndEntity = 0, MustBeCA = 1 };

typedef unsigned int KeyUsages;

MOZILLA_PKIX_ENUM_CLASS KeyPurposeId {
  anyExtendedKeyUsage = 0,
  id_kp_serverAuth = 1,           
  id_kp_clientAuth = 2,           
  id_kp_codeSigning = 3,          
  id_kp_emailProtection = 4,      
  id_kp_OCSPSigning = 9,          
};

struct CertPolicyId {
  uint16_t numBytes;
  static const uint16_t MAX_BYTES = 24;
  uint8_t bytes[MAX_BYTES];

  bool IsAnyPolicy() const;

  static const CertPolicyId anyPolicy;
};

MOZILLA_PKIX_ENUM_CLASS TrustLevel {
  TrustAnchor = 1,        
                          
  ActivelyDistrusted = 2, 
  InheritsTrust = 3       
};





class TrustDomain
{
public:
  virtual ~TrustDomain() { }

  
  
  
  
  
  
  
  
  
  
  
  
  virtual SECStatus GetCertTrust(EndEntityOrCA endEntityOrCA,
                                 const CertPolicyId& policy,
                                 const SECItem& candidateCertDER,
                          TrustLevel* trustLevel) = 0;

  
  
  
  
  
  
  
  
  virtual SECStatus FindPotentialIssuers(const SECItem* encodedIssuerName,
                                         PRTime time,
                                  ScopedCERTCertList& results) = 0;

  
  
  
  
  
  
  
  virtual SECStatus VerifySignedData(const CERTSignedData* signedData,
                                     const CERTCertificate* cert) = 0;

  
  
  virtual SECStatus CheckRevocation(EndEntityOrCA endEntityOrCA,
                                    const CERTCertificate* cert,
                           CERTCertificate* issuerCertToDup,
                                    PRTime time,
                        const SECItem* stapledOCSPresponse) = 0;

  
  
  
  virtual SECStatus IsChainValid(const CERTCertList* certChain) = 0;

protected:
  TrustDomain() { }

private:
  TrustDomain(const TrustDomain&) ;
  void operator=(const TrustDomain&) ;
};

} } 

#endif
