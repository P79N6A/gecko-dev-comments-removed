





#ifndef mozilla_psm__CertVerifier_h
#define mozilla_psm__CertVerifier_h

#include "pkix/pkixtypes.h"
#include "OCSPCache.h"

namespace mozilla { namespace psm {

struct ChainValidationCallbackState;

class CertVerifier
{
public:
  typedef unsigned int Flags;
  
  static const Flags FLAG_LOCAL_ONLY;
  
  static const Flags FLAG_MUST_BE_EV;

  
  
  SECStatus VerifyCert(CERTCertificate* cert,
                       const SECCertificateUsage usage,
                       const PRTime time,
                       void* pinArg,
                       const char* hostname,
                       const Flags flags = 0,
        const SECItem* stapledOCSPResponse = nullptr,
       mozilla::pkix::ScopedCERTCertList* validationChain = nullptr,
       SECOidTag* evOidPolicy = nullptr ,
       CERTVerifyLog* verifyLog = nullptr);

  SECStatus VerifySSLServerCert(
                    CERTCertificate* peerCert,
        const SECItem* stapledOCSPResponse,
                    PRTime time,
        void* pinarg,
                    const char* hostname,
                    bool saveIntermediatesInPermanentDatabase = false,
    mozilla::pkix::ScopedCERTCertList* certChainOut = nullptr,
    SECOidTag* evOidPolicy = nullptr);


  enum implementation_config {
    classic = 0,
#ifndef NSS_NO_LIBPKIX
    libpkix = 1,
#endif
    mozillapkix = 2
  };

  enum pinning_enforcement_config {
    pinningDisabled = 0,
    pinningAllowUserCAMITM = 1,
    pinningStrict = 2,
    pinningEnforceTestMode = 3
  };

  enum missing_cert_download_config { missing_cert_download_off = 0, missing_cert_download_on };
  enum crl_download_config { crl_local_only = 0, crl_download_allowed };
  enum ocsp_download_config { ocsp_off = 0, ocsp_on };
  enum ocsp_strict_config { ocsp_relaxed = 0, ocsp_strict };
  enum ocsp_get_config { ocsp_get_disabled = 0, ocsp_get_enabled = 1 };

  bool IsOCSPDownloadEnabled() const { return mOCSPDownloadEnabled; }

  CertVerifier(implementation_config ic,
#ifndef NSS_NO_LIBPKIX
               missing_cert_download_config ac, crl_download_config cdc,
#endif
               ocsp_download_config odc, ocsp_strict_config osc,
               ocsp_get_config ogc,
               pinning_enforcement_config pinningEnforcementLevel);
  ~CertVerifier();

  void ClearOCSPCache() { mOCSPCache.Clear(); }

  const implementation_config mImplementation;
#ifndef NSS_NO_LIBPKIX
  const bool mMissingCertDownloadEnabled;
  const bool mCRLDownloadEnabled;
#endif
  const bool mOCSPDownloadEnabled;
  const bool mOCSPStrict;
  const bool mOCSPGETEnabled;
  const pinning_enforcement_config mPinningEnforcementLevel;

private:
  SECStatus MozillaPKIXVerifyCert(CERTCertificate* cert,
      const SECCertificateUsage usage,
      const PRTime time,
      void* pinArg,
      const Flags flags,
      ChainValidationCallbackState* callbackState,
       const SECItem* stapledOCSPResponse,
       mozilla::pkix::ScopedCERTCertList* validationChain,
       SECOidTag* evOidPolicy);

  OCSPCache mOCSPCache;
};

void InitCertVerifierLog();
} } 

#endif 
