





#ifndef mozilla_psm__NSSCertDBTrustDomain_h
#define mozilla_psm__NSSCertDBTrustDomain_h

#include "CertVerifier.h"
#include "nsICertBlocklist.h"
#include "pkix/pkixtypes.h"
#include "secmodt.h"

namespace mozilla { namespace psm {

enum class ValidityCheckingMode {
  CheckingOff = 0,
  CheckForEV = 1,
};

SECStatus InitializeNSS(const char* dir, bool readOnly);

void DisableMD5();

extern const char BUILTIN_ROOTS_MODULE_DEFAULT_NAME[];







SECStatus LoadLoadableRoots( const char* dir,
                            const char* modNameUTF8);

void UnloadLoadableRoots(const char* modNameUTF8);


char* DefaultServerNicknameForCert(CERTCertificate* cert);

void SaveIntermediateCerts(const ScopedCERTCertList& certList);

enum SignatureDigestOption {
  AcceptAllAlgorithms,
  DisableSHA1ForEE,
  DisableSHA1ForCA,
  DisableSHA1Everywhere,
};

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
                       CertVerifier::OcspGetConfig ocspGETConfig,
                       uint32_t certShortLifetimeInDays,
                       CertVerifier::PinningMode pinningMode,
                       unsigned int minRSABits,
                       ValidityCheckingMode validityCheckingMode,
                       SignatureDigestOption,
           const char* hostname = nullptr,
       ScopedCERTCertList* builtChain = nullptr);

  virtual Result FindIssuer(mozilla::pkix::Input encodedIssuerName,
                            IssuerChecker& checker,
                            mozilla::pkix::Time time) override;

  virtual Result GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                              const mozilla::pkix::CertPolicyId& policy,
                              mozilla::pkix::Input candidateCertDER,
                               mozilla::pkix::TrustLevel& trustLevel)
                              override;

  virtual Result CheckSignatureDigestAlgorithm(
                   mozilla::pkix::DigestAlgorithm digestAlg,
                   mozilla::pkix::EndEntityOrCA endEntityOrCA) override;

  virtual Result CheckRSAPublicKeyModulusSizeInBits(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   unsigned int modulusSizeInBits) override;

  virtual Result VerifyRSAPKCS1SignedDigest(
                   const mozilla::pkix::SignedDigest& signedDigest,
                   mozilla::pkix::Input subjectPublicKeyInfo) override;

  virtual Result CheckECDSACurveIsAcceptable(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   mozilla::pkix::NamedCurve curve) override;

  virtual Result VerifyECDSASignedDigest(
                   const mozilla::pkix::SignedDigest& signedDigest,
                   mozilla::pkix::Input subjectPublicKeyInfo) override;

  virtual Result DigestBuf(mozilla::pkix::Input item,
                           mozilla::pkix::DigestAlgorithm digestAlg,
                            uint8_t* digestBuf,
                           size_t digestBufLen) override;

  virtual Result CheckValidityIsAcceptable(
                   mozilla::pkix::Time notBefore, mozilla::pkix::Time notAfter,
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   mozilla::pkix::KeyPurposeId keyPurpose) override;

  virtual Result CheckRevocation(
                   mozilla::pkix::EndEntityOrCA endEntityOrCA,
                   const mozilla::pkix::CertID& certID,
                   mozilla::pkix::Time time,
                   mozilla::pkix::Duration validityDuration,
       const mozilla::pkix::Input* stapledOCSPResponse,
       const mozilla::pkix::Input* aiaExtension)
                   override;

  virtual Result IsChainValid(const mozilla::pkix::DERArray& certChain,
                              mozilla::pkix::Time time) override;

  CertVerifier::OCSPStaplingStatus GetOCSPStaplingStatus() const
  {
    return mOCSPStaplingStatus;
  }
  void ResetOCSPStaplingStatus()
  {
    mOCSPStaplingStatus = CertVerifier::OCSP_STAPLING_NEVER_CHECKED;
  }

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
  const CertVerifier::OcspGetConfig mOCSPGetConfig;
  const uint32_t mCertShortLifetimeInDays;
  CertVerifier::PinningMode mPinningMode;
  const unsigned int mMinRSABits;
  ValidityCheckingMode mValidityCheckingMode;
  SignatureDigestOption mSignatureDigestOption;
  const char* mHostname; 
  ScopedCERTCertList* mBuiltChain; 
  nsCOMPtr<nsICertBlocklist> mCertBlocklist;
  CertVerifier::OCSPStaplingStatus mOCSPStaplingStatus;
};

} } 

#endif
