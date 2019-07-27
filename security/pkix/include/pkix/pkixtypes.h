























#ifndef mozilla_pkix__pkixtypes_h
#define mozilla_pkix__pkixtypes_h

#include "cert.h"
#include "pkix/enumclass.h"
#include "seccomon.h"
#include "secport.h"
#include "stdint.h"

namespace mozilla { namespace pkix {

MOZILLA_PKIX_ENUM_CLASS EndEntityOrCA { MustBeEndEntity = 0, MustBeCA = 1 };

MOZILLA_PKIX_ENUM_CLASS KeyUsage : uint8_t {
  digitalSignature = 0,
  nonRepudiation   = 1,
  keyEncipherment  = 2,
  dataEncipherment = 3,
  keyAgreement     = 4,
  keyCertSign      = 5,
  
  
  
  noParticularKeyUsageRequired = 0xff,
};

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












struct CertID
{
public:
  CertID(const SECItem& issuer, const SECItem& issuerSubjectPublicKeyInfo,
         const SECItem& serialNumber)
    : issuer(issuer)
    , issuerSubjectPublicKeyInfo(issuerSubjectPublicKeyInfo)
    , serialNumber(serialNumber)
  {
  }
  const SECItem& issuer;
  const SECItem& issuerSubjectPublicKeyInfo;
  const SECItem& serialNumber;
private:
  void operator=(const CertID&) ;
};

class DERArray
{
public:
  
  virtual size_t GetLength() const = 0;

  
  
  
  virtual const SECItem* GetDER(size_t i) const = 0;
protected:
  DERArray() { }
  virtual ~DERArray() { }
};





class TrustDomain
{
public:
  virtual ~TrustDomain() { }

  
  
  
  
  
  
  
  
  
  
  
  
  virtual SECStatus GetCertTrust(EndEntityOrCA endEntityOrCA,
                                 const CertPolicyId& policy,
                                 const SECItem& candidateCertDER,
                          TrustLevel* trustLevel) = 0;

  class IssuerChecker
  {
  public:
    virtual SECStatus Check(const SECItem& potentialIssuerDER,
                             bool& keepGoing) = 0;
  protected:
    IssuerChecker();
    virtual ~IssuerChecker();
  private:
    IssuerChecker(const IssuerChecker&) ;
    void operator=(const IssuerChecker&) ;
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual SECStatus FindIssuer(const SECItem& encodedIssuerName,
                               IssuerChecker& checker, PRTime time) = 0;

  
  
  
  
  virtual SECStatus VerifySignedData(const CERTSignedData& signedData,
                                     const SECItem& subjectPublicKeyInfo) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual SECStatus IsChainValid(const DERArray& certChain) = 0;

  
  
  virtual SECStatus CheckRevocation(EndEntityOrCA endEntityOrCA,
                                    const CertID& certID, PRTime time,
                        const SECItem* stapledOCSPresponse,
                        const SECItem* aiaExtension) = 0;

protected:
  TrustDomain() { }

private:
  TrustDomain(const TrustDomain&) ;
  void operator=(const TrustDomain&) ;
};

} } 

#endif
