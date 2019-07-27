



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

  SharedCertVerifier(OcspDownloadConfig odc, OcspStrictConfig osc,
                     OcspGetConfig ogc, uint32_t certShortLifetimeInDays,
                     PinningMode pinningMode)
    : mozilla::psm::CertVerifier(odc, osc, ogc, certShortLifetimeInDays,
                                 pinningMode)
  {
  }
};

} } 

#endif 
