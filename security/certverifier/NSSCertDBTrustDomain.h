





#ifndef mozilla_psm__NSSCertDBTrustDomain_h
#define mozilla_psm__NSSCertDBTrustDomain_h

#include "pkix/pkixtypes.h"
#include "secmodt.h"
#include "CertVerifier.h"

namespace mozilla { namespace psm {

SECStatus InitializeNSS(const char* dir, bool readOnly);

void DisableMD5();

extern const char BUILTIN_ROOTS_MODULE_DEFAULT_NAME[];

void PORT_Free_string(char* str);







SECStatus LoadLoadableRoots( const char* dir,
                            const char* modNameUTF8);

void UnloadLoadableRoots(const char* modNameUTF8);


char* DefaultServerNicknameForCert(CERTCertificate* cert);

void SaveIntermediateCerts(const ScopedCERTCertList& certList);

class NSSCertDBTrustDomain : public mozilla::pkix::TrustDomain
{

public:
  typedef mozilla::pkix::Result Result;

  enum OCSPFetching {
    NeverFetchOCSP = 0,
    FetchOCSPForDVSoftFail = 1,
    FetchOCSPForDVHardFail = 2,
    FetchOCSPForEV = 3,
    LocalOnlyOCSPForEV = 4,
  };
  NSSCertDBTrustDomain(SECTrustType certDBTrustType, OCSPFetching ocspFetching,
                       OCSPCache& ocspCache, void* pinArg,
                       CertVerifier::ocsp_get_config ocspGETConfig,
                       CertVerifier::PinningMode pinningMode,
                       bool forEV,
           const char* hostname = nullptr,
       ScopedCERTCertList* builtChain = nullptr);

  virtual Result FindIssuer(mozilla::pkix::Input encodedIssuerName,
                            IssuerChecker& checker,
                            mozilla::pkix::Time time) MOZ_OVERRIDE;

  virtual Result GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                              const mozilla::pkix::CertPolicyId& policy,
                              mozilla::pkix::Input candidateCertDER,
                               mozilla::pkix::TrustLevel& trustLevel)
                              MOZ_OVERRIDE;

  virtual Result CheckPublicKey(mozilla::pkix::Input subjectPublicKeyInfo)
                                MOZ_OVERRIDE;

  virtual Result VerifySignedData(
                   const mozilla::pkix::SignedDataWithSignature& signedData,
                   mozilla::pkix::Input subjectPublicKeyInfo)
                   MOZ_OVERRIDE;

  virtual Result DigestBuf(mozilla::pkix::Input item,
                            uint8_t* digestBuf,
                           size_t digestBufLen) MOZ_OVERRIDE;

  virtual Result CheckRevocation(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   const mozilla::pkix::CertID& certID,
                   mozilla::pkix::Time time,
       const mozilla::pkix::Input* stapledOCSPResponse,
       const mozilla::pkix::Input* aiaExtension)
                   MOZ_OVERRIDE;

  virtual Result IsChainValid(const mozilla::pkix::DERArray& certChain,
                              mozilla::pkix::Time time) MOZ_OVERRIDE;

private:
  enum EncodedResponseSource {
    ResponseIsFromNetwork = 1,
    ResponseWasStapled = 2
  };
  Result VerifyAndMaybeCacheEncodedOCSPResponse(
    const mozilla::pkix::CertID& certID, mozilla::pkix::Time time,
    uint16_t maxLifetimeInDays, mozilla::pkix::Input encodedResponse,
    EncodedResponseSource responseSource,  bool& expired);

  const SECTrustType mCertDBTrustType;
  const OCSPFetching mOCSPFetching;
  OCSPCache& mOCSPCache; 
  void* mPinArg; 
  const CertVerifier::ocsp_get_config mOCSPGetConfig;
  CertVerifier::PinningMode mPinningMode;
  const unsigned int mMinimumNonECCBits;
  const char* mHostname; 
  ScopedCERTCertList* mBuiltChain; 
};

} } 

#endif
