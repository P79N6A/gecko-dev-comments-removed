























#ifndef mozilla_pkix__pkixtypes_h
#define mozilla_pkix__pkixtypes_h

#include "pkix/enumclass.h"
#include "pkix/ScopedPtr.h"
#include "plarena.h"
#include "cert.h"
#include "keyhi.h"

namespace mozilla { namespace pkix {

typedef ScopedPtr<PLArenaPool, PL_FreeArenaPool> ScopedPLArenaPool;

typedef ScopedPtr<CERTCertificate, CERT_DestroyCertificate>
        ScopedCERTCertificate;
typedef ScopedPtr<CERTCertList, CERT_DestroyCertList> ScopedCERTCertList;
typedef ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
        ScopedSECKEYPublicKey;

typedef unsigned int KeyUsages;

MOZILLA_PKIX_ENUM_CLASS EndEntityOrCA { MustBeEndEntity = 0, MustBeCA = 1 };

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
                                 SECOidTag policy,
                                 const CERTCertificate* candidateCert,
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
