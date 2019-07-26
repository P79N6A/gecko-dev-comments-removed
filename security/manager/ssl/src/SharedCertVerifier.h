



#ifndef mozilla_psm__SharedCertVerifier_h
#define mozilla_psm__SharedCertVerifier_h

#include "certt.h"
#include "CertVerifier.h"
#include "mozilla/RefPtr.h"

namespace mozilla { namespace psm {

class SharedCertVerifier : public mozilla::psm::CertVerifier,
                           public mozilla::AtomicRefCounted<SharedCertVerifier>
{
public:
  SharedCertVerifier(implementation_config ic,
#ifndef NSS_NO_LIBPKIX
                     missing_cert_download_config ac, crl_download_config cdc,
#endif
                     ocsp_download_config odc, ocsp_strict_config osc,
                     ocsp_get_config ogc)
    : mozilla::psm::CertVerifier(ic,
#ifndef NSS_NO_LIBPKIX
                                 ac, cdc,
#endif
                                 odc, osc, ogc)
  {
  }
  ~SharedCertVerifier();
};

} } 

#endif 
