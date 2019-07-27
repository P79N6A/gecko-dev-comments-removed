























#ifndef mozilla_pkix_pkixtypes_h
#define mozilla_pkix_pkixtypes_h

#include "pkix/Input.h"
#include "pkix/Time.h"
#include "stdint.h"

namespace mozilla { namespace pkix {

enum class DigestAlgorithm
{
  sha512 = 1,
  sha384 = 2,
  sha256 = 3,
  sha1 = 4,
};

enum class NamedCurve
{
  
  secp521r1 = 1,

  
  secp384r1 = 2,

  
  secp256r1 = 3,
};

struct SignedDigest final
{
  Input digest;
  DigestAlgorithm digestAlgorithm;
  Input signature;

  void operator=(const SignedDigest&) = delete;
};

enum class EndEntityOrCA { MustBeEndEntity = 0, MustBeCA = 1 };

enum class KeyUsage : uint8_t
{
  digitalSignature = 0,
  nonRepudiation   = 1,
  keyEncipherment  = 2,
  dataEncipherment = 3,
  keyAgreement     = 4,
  keyCertSign      = 5,
  
  
  
  noParticularKeyUsageRequired = 0xff,
};

enum class KeyPurposeId
{
  anyExtendedKeyUsage = 0,
  id_kp_serverAuth = 1,           
  id_kp_clientAuth = 2,           
  id_kp_codeSigning = 3,          
  id_kp_emailProtection = 4,      
  id_kp_OCSPSigning = 9,          
};

struct CertPolicyId final
{
  uint16_t numBytes;
  static const uint16_t MAX_BYTES = 24;
  uint8_t bytes[MAX_BYTES];

  bool IsAnyPolicy() const;

  static const CertPolicyId anyPolicy;
};

enum class TrustLevel
{
  TrustAnchor = 1,        
                          
  ActivelyDistrusted = 2, 
  InheritsTrust = 3       
};












struct CertID final
{
public:
  CertID(Input issuer, Input issuerSubjectPublicKeyInfo, Input serialNumber)
    : issuer(issuer)
    , issuerSubjectPublicKeyInfo(issuerSubjectPublicKeyInfo)
    , serialNumber(serialNumber)
  {
  }
  const Input issuer;
  const Input issuerSubjectPublicKeyInfo;
  const Input serialNumber;

  void operator=(const CertID&) = delete;
};

class DERArray
{
public:
  
  virtual size_t GetLength() const = 0;

  
  
  
  virtual const Input* GetDER(size_t i) const = 0;
protected:
  DERArray() { }
  virtual ~DERArray() { }
};





class TrustDomain
{
public:
  virtual ~TrustDomain() { }

  
  
  
  
  
  
  
  
  
  
  
  
  virtual Result GetCertTrust(EndEntityOrCA endEntityOrCA,
                              const CertPolicyId& policy,
                              Input candidateCertDER,
                               TrustLevel& trustLevel) = 0;

  class IssuerChecker
  {
  public:
    
    
    
    
    
    
    
    virtual Result Check(Input potentialIssuerDER,
             const Input* additionalNameConstraints,
                  bool& keepGoing) = 0;
  protected:
    IssuerChecker();
    virtual ~IssuerChecker();

    IssuerChecker(const IssuerChecker&) = delete;
    void operator=(const IssuerChecker&) = delete;
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual Result FindIssuer(Input encodedIssuerName,
                            IssuerChecker& checker, Time time) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual Result IsChainValid(const DERArray& certChain, Time time) = 0;

  virtual Result CheckRevocation(EndEntityOrCA endEntityOrCA,
                                 const CertID& certID, Time time,
                     const Input* stapledOCSPresponse,
                     const Input* aiaExtension) = 0;

  
  
  
  
  
  virtual Result CheckSignatureDigestAlgorithm(DigestAlgorithm digestAlg) = 0;

  
  
  
  
  
  virtual Result CheckRSAPublicKeyModulusSizeInBits(
                   EndEntityOrCA endEntityOrCA,
                   unsigned int modulusSizeInBits) = 0;

  
  
  
  
  
  
  
  virtual Result VerifyRSAPKCS1SignedDigest(
                   const SignedDigest& signedDigest,
                   Input subjectPublicKeyInfo) = 0;

  
  
  
  
  
  virtual Result CheckECDSACurveIsAcceptable(EndEntityOrCA endEntityOrCA,
                                             NamedCurve curve) = 0;

  
  
  
  
  
  
  
  virtual Result VerifyECDSASignedDigest(const SignedDigest& signedDigest,
                                         Input subjectPublicKeyInfo) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual Result DigestBuf(Input item,
                           DigestAlgorithm digestAlg,
                            uint8_t* digestBuf,
                           size_t digestBufLen) = 0;
protected:
  TrustDomain() { }

  TrustDomain(const TrustDomain&) = delete;
  void operator=(const TrustDomain&) = delete;
};

} } 

#endif
