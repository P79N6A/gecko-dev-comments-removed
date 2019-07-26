





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
  NSSCertDBTrustDomain(SECTrustType certDBTrustType,
                       bool ocspDownloadEnabled, bool ocspStrict,
                       void* pinArg);

  virtual SECStatus FindPotentialIssuers(
                        const SECItem* encodedIssuerName,
                        PRTime time,
                 insanity::pkix::ScopedCERTCertList& results);

  virtual SECStatus GetCertTrust(insanity::pkix::EndEntityOrCA endEntityOrCA,
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
  const bool mOCSPDownloadEnabled;
  const bool mOCSPStrict;
  void* mPinArg; 
};

} } 

#endif 
