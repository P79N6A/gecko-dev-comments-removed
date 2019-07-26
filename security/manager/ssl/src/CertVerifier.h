



#ifndef mozilla_psm__CertVerifier_h
#define mozilla_psm__CertVerifier_h

#include "mozilla/RefPtr.h"
#include "CryptoUtil.h"
#include "nsISupportsImpl.h"
#include "certt.h"

class nsIInterfaceRequestor;
class nsNSSComponent;

namespace mozilla { namespace psm {

class CertVerifier
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CertVerifier)

  typedef unsigned int Flags;
  static const Flags FLAG_LOCAL_ONLY;
  

  
  SECStatus VerifyCert(CERTCertificate * cert,
                       const SECCertificateUsage usage,
                       const PRTime time,
                       nsIInterfaceRequestor * pinArg,
                       const Flags flags = 0,
                        CERTCertList **validationChain = nullptr,
                        SECOidTag *evOidPolicy = nullptr ,
                        CERTVerifyLog *verifyLog = nullptr);

  enum missing_cert_download_config { missing_cert_download_off = 0, missing_cert_download_on };
  enum crl_download_config { crl_local_only = 0, crl_download_allowed };
  enum ocsp_download_config { ocsp_off = 0, ocsp_on };
  enum ocsp_strict_config { ocsp_relaxed = 0, ocsp_strict };
  enum any_revo_fresh_config { any_revo_relaxed = 0, any_revo_strict };
  enum ocsp_get_config { ocsp_get_disabled = 0, ocsp_get_enabled = 1 };

  bool IsOCSPDownloadEnabled() const { return mOCSPDownloadEnabled; }

private:
  CertVerifier(missing_cert_download_config ac, crl_download_config cdc,
               ocsp_download_config odc, ocsp_strict_config osc,
               any_revo_fresh_config arfc,
               const char *firstNetworkRevocationMethod,
               ocsp_get_config ogc);
  ~CertVerifier();

  const bool mMissingCertDownloadEnabled;
  const bool mCRLDownloadEnabled;
  const bool mOCSPDownloadEnabled;
  const bool mOCSPStrict;
  const bool mRequireRevocationInfo;
  const bool mCRLFirst;
  const bool mOCSPGETEnabled;
  friend class ::nsNSSComponent;
};

MOZ_WARN_UNUSED_RESULT TemporaryRef<CertVerifier> GetDefaultCertVerifier();

} } 

#endif 
