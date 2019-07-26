





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




void
SetClassicOCSPBehavior(CertVerifier::ocsp_download_config enabled,
                       CertVerifier::ocsp_strict_config strict,
                       CertVerifier::ocsp_get_config get);


char* DefaultServerNicknameForCert(CERTCertificate* cert);

void SaveIntermediateCerts(const mozilla::pkix::ScopedCERTCertList& certList);

class NSSCertDBTrustDomain : public mozilla::pkix::TrustDomain
{

public:
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
                       CERTChainVerifyCallback* checkChainCallback = nullptr);

  virtual SECStatus FindPotentialIssuers(
                        const SECItem* encodedIssuerName,
                        PRTime time,
                 mozilla::pkix::ScopedCERTCertList& results);

  virtual SECStatus GetCertTrust(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                                 const mozilla::pkix::CertPolicyId& policy,
                                 const CERTCertificate* candidateCert,
                          mozilla::pkix::TrustLevel* trustLevel);

  virtual SECStatus VerifySignedData(const CERTSignedData* signedData,
                                     const CERTCertificate* cert);

  virtual SECStatus CheckRevocation(mozilla::pkix::EndEntityOrCA endEntityOrCA,
                                    const CERTCertificate* cert,
                           CERTCertificate* issuerCert,
                                    PRTime time,
                        const SECItem* stapledOCSPResponse);

  virtual SECStatus IsChainValid(const CERTCertList* certChain);

private:
  enum EncodedResponseSource {
    ResponseIsFromNetwork = 1,
    ResponseWasStapled = 2
  };
  static const PRTime ServerFailureDelay = 5 * 60 * PR_USEC_PER_SEC;
  SECStatus VerifyAndMaybeCacheEncodedOCSPResponse(
    const CERTCertificate* cert, CERTCertificate* issuerCert, PRTime time,
    uint16_t maxLifetimeInDays, const SECItem* encodedResponse,
    EncodedResponseSource responseSource);

  const SECTrustType mCertDBTrustType;
  const OCSPFetching mOCSPFetching;
  OCSPCache& mOCSPCache; 
  void* mPinArg; 
  const CertVerifier::ocsp_get_config mOCSPGetConfig;
  CERTChainVerifyCallback* mCheckChainCallback; 
};

} } 

#endif
