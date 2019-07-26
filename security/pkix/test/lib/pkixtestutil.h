
















#ifndef mozilla_pkix_test__pkixtestutils_h
#define mozilla_pkix_test__pkixtestutils_h

#include "pkix/ScopedPtr.h"
#include "pkix/pkixtypes.h"
#include "seccomon.h"

namespace mozilla { namespace pkix { namespace test {

class OCSPResponseContext
{
public:
  OCSPResponseContext(PLArenaPool* arena, CERTCertificate* cert, PRTime time);

  PLArenaPool* arena;
  
  pkix::ScopedCERTCertificate cert; 
  pkix::ScopedCERTCertificate issuerCert; 
  pkix::ScopedCERTCertificate signerCert; 
  uint8_t responseStatus; 
  bool skipResponseBytes; 

  
  
  PRTime producedAt;
  PRTime thisUpdate;
  PRTime nextUpdate;
  bool includeNextUpdate;
  SECOidTag certIDHashAlg;
  uint8_t certStatus;     
  PRTime revocationTime; 
  bool badSignature; 

  enum ResponderIDType {
    ByName = 1,
    ByKeyHash = 2
  };
  ResponderIDType responderIDType;
};






SECItem* CreateEncodedOCSPResponse(OCSPResponseContext& context);

} } } 

#endif 
