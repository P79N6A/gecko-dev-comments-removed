





#ifndef mozilla_psm__CertVerifier_h
#define mozilla_psm__CertVerifier_h

#include "pkix/pkixtypes.h"
#include "OCSPCache.h"
#include "ScopedNSSTypes.h"

namespace mozilla { namespace psm {

struct ChainValidationCallbackState;

class CertVerifier
{
public:
  typedef unsigned int Flags;
  
  static const Flags FLAG_LOCAL_ONLY;
  
  static const Flags FLAG_MUST_BE_EV;

  
  enum OCSPStaplingStatus {
    OCSP_STAPLING_NEVER_CHECKED = 0,
    OCSP_STAPLING_GOOD = 1,
    OCSP_STAPLING_NONE = 2,
    OCSP_STAPLING_EXPIRED = 3,
    OCSP_STAPLING_INVALID = 4,
  };

  
  
  SECStatus VerifyCert(CERTCertificate* cert,
                       SECCertificateUsage usage,
                       mozilla::pkix::Time time,
                       void* pinArg,
                       const char* hostname,
                       Flags flags = 0,
        const SECItem* stapledOCSPResponse = nullptr,
       ScopedCERTCertList* builtChain = nullptr,
       SECOidTag* evOidPolicy = nullptr,
       OCSPStaplingStatus* ocspStaplingStatus = nullptr);

  SECStatus VerifySSLServerCert(
                    CERTCertificate* peerCert,
        const SECItem* stapledOCSPResponse,
                    mozilla::pkix::Time time,
        void* pinarg,
                    const char* hostname,
                    bool saveIntermediatesInPermanentDatabase = false,
                    Flags flags = 0,
    ScopedCERTCertList* builtChain = nullptr,
    SECOidTag* evOidPolicy = nullptr,
    OCSPStaplingStatus* ocspStaplingStatus = nullptr);

  enum PinningMode {
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

  CertVerifier(ocsp_download_config odc, ocsp_strict_config osc,
               ocsp_get_config ogc, PinningMode pinningMode);
  ~CertVerifier();

  void ClearOCSPCache() { mOCSPCache.Clear(); }

  const bool mOCSPDownloadEnabled;
  const bool mOCSPStrict;
  const bool mOCSPGETEnabled;
  const PinningMode mPinningMode;

private:
  OCSPCache mOCSPCache;
};

void InitCertVerifierLog();
SECStatus IsCertBuiltInRoot(CERTCertificate* cert, bool& result);
mozilla::pkix::Result CertListContainsExpectedKeys(
  const CERTCertList* certList, const char* hostname, mozilla::pkix::Time time,
  CertVerifier::PinningMode pinningMode);

} } 

#endif 
