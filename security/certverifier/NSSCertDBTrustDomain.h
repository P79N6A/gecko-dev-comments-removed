





#ifndef mozilla_psm__NSSCertDBTrustDomain_h
#define mozilla_psm__NSSCertDBTrustDomain_h

#include "insanity/pkixtypes.h"
#include "secmodt.h"
#include "CertVerifier.h"

namespace mozilla { namespace psm {

SECStatus InitializeNSS(const char* dir, bool readOnly);

void DisableMD5();

extern const char BUILTIN_ROOTS_MODULE_DEFAULT_NAME[];







SECStatus LoadLoadableRoots( const char* dir,
                            const char* modNameUTF8);

void UnloadLoadableRoots(const char* modNameUTF8);




void
SetClassicOCSPBehavior(CertVerifier::ocsp_download_config enabled,
                       CertVerifier::ocsp_strict_config strict,
                       CertVerifier::ocsp_get_config get);


char* DefaultServerNicknameForCert(CERTCertificate* cert);

void SaveIntermediateCerts(const insanity::pkix::ScopedCERTCertList& certList);

class NSSCertDBTrustDomain : public insanity::pkix::TrustDomain
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
                       void* pinArg);

  virtual SECStatus FindPotentialIssuers(
                        const SECItem* encodedIssuerName,
                        PRTime time,
                 insanity::pkix::ScopedCERTCertList& results);

  virtual SECStatus GetCertTrust(insanity::pkix::EndEntityOrCA endEntityOrCA,
                                 SECOidTag policy,
                                 const CERTCertificate* candidateCert,
                          TrustLevel* trustLevel);

  virtual SECStatus VerifySignedData(const CERTSignedData* signedData,
                                     const CERTCertificate* cert);

  virtual SECStatus CheckRevocation(insanity::pkix::EndEntityOrCA endEntityOrCA,
                                    const CERTCertificate* cert,
                           CERTCertificate* issuerCert,
                                    PRTime time,
                        const SECItem* stapledOCSPResponse);

private:
  const SECTrustType mCertDBTrustType;
  const OCSPFetching mOCSPFetching;
  void* mPinArg; 
};

} } 

#endif
