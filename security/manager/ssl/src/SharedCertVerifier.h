



#ifndef mozilla_psm__SharedCertVerifier_h
#define mozilla_psm__SharedCertVerifier_h

#include "certt.h"
#include "CertVerifier.h"
#include "mozilla/RefPtr.h"

namespace mozilla { namespace psm {

class SharedCertVerifier : public mozilla::psm::CertVerifier
{
protected:
  ~SharedCertVerifier();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SharedCertVerifier)

  SharedCertVerifier(ocsp_download_config odc, ocsp_strict_config osc,
                     ocsp_get_config ogc,
                     pinning_enforcement_config pinningEnforcementLevel)
    : mozilla::psm::CertVerifier(odc, osc, ogc, pinningEnforcementLevel)
  {
  }
};

} } 

#endif 
