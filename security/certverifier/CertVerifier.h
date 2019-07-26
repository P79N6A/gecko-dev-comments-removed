





#ifndef mozilla_psm__CertVerifier_h
#define mozilla_psm__CertVerifier_h

#include "certt.h"
#include "insanity/pkixtypes.h"

namespace mozilla { namespace psm {

class CertVerifier
{
public:
  typedef unsigned int Flags;
  
  static const Flags FLAG_LOCAL_ONLY;
  
  static const Flags FLAG_NO_DV_FALLBACK_FOR_EV;

  
  
  SECStatus VerifyCert(CERTCertificate* cert,
           const SECItem* stapledOCSPResponse,
                       const SECCertificateUsage usage,
                       const PRTime time,
                       void* pinArg,
                       const Flags flags = 0,
       insanity::pkix::ScopedCERTCertList* validationChain = nullptr,
       SECOidTag* evOidPolicy = nullptr ,
       CERTVerifyLog* verifyLog = nullptr);

  SECStatus VerifySSLServerCert(
                    CERTCertificate* peerCert,
        const SECItem* stapledOCSPResponse,
                    PRTime time,
        void* pinarg,
                    const char* hostname,
                    bool saveIntermediatesInPermanentDatabase = false,
    insanity::pkix::ScopedCERTCertList* certChainOut = nullptr,
    SECOidTag* evOidPolicy = nullptr);


  enum implementation_config {
    classic = 0,
#ifndef NSS_NO_LIBPKIX
    libpkix = 1,
#endif
  };

  enum missing_cert_download_config { missing_cert_download_off = 0, missing_cert_download_on };
  enum crl_download_config { crl_local_only = 0, crl_download_allowed };
  enum ocsp_download_config { ocsp_off = 0, ocsp_on };
  enum ocsp_strict_config { ocsp_relaxed = 0, ocsp_strict };
  enum ocsp_get_config { ocsp_get_disabled = 0, ocsp_get_enabled = 1 };

  bool IsOCSPDownloadEnabled() const { return mOCSPDownloadEnabled; }

  CertVerifier(implementation_config ic, missing_cert_download_config ac,
               crl_download_config cdc, ocsp_download_config odc,
               ocsp_strict_config osc, ocsp_get_config ogc);
  ~CertVerifier();

public:
  const implementation_config mImplementation;
  const bool mMissingCertDownloadEnabled;
  const bool mCRLDownloadEnabled;
  const bool mOCSPDownloadEnabled;
  const bool mOCSPStrict;
  const bool mOCSPGETEnabled;
};

void InitCertVerifierLog();
} } 

#endif 
