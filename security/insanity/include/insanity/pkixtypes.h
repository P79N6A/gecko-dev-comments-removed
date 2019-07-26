
















#ifndef insanity_pkix__pkixtypes_h
#define insanity_pkix__pkixtypes_h

#include "insanity/ScopedPtr.h"
#include "plarena.h"
#include "cert.h"
#include "keyhi.h"

namespace insanity { namespace pkix {

typedef ScopedPtr<PLArenaPool, PL_FreeArenaPool> ScopedPLArenaPool;

typedef ScopedPtr<CERTCertificate, CERT_DestroyCertificate>
        ScopedCERTCertificate;
typedef ScopedPtr<CERTCertList, CERT_DestroyCertList> ScopedCERTCertList;
typedef ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
        ScopedSECKEYPublicKey;

typedef unsigned int KeyUsages;

enum EndEntityOrCA { MustBeEndEntity, MustBeCA };





class TrustDomain
{
public:
  virtual ~TrustDomain() { }

  enum TrustLevel {
    TrustAnchor = 1,        
                            
    ActivelyDistrusted = 2, 
    InheritsTrust = 3       
  };

  
  
  
  virtual SECStatus GetCertTrust(EndEntityOrCA endEntityOrCA,
                                 const CERTCertificate* candidateCert,
                          TrustLevel* trustLevel) = 0;

  
  
  
  
  
  virtual SECStatus FindPotentialIssuers(const SECItem* encodedIssuerName,
                                         PRTime time,
                                  ScopedCERTCertList& results) = 0;

  
  
  
  
  
  
  
  virtual SECStatus VerifySignedData(const CERTSignedData* signedData,
                                     const CERTCertificate* cert) = 0;

protected:
  TrustDomain() { }

private:
  TrustDomain(const TrustDomain&) ;
  void operator=(const TrustDomain&) ;
};

} } 

#endif 
