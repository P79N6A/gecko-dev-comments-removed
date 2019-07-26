
















#ifndef insanity_test__pkixtestutils_h
#define insanity_test__pkixtestutils_h

#include "insanity/ScopedPtr.h"
#include "insanity/pkixtypes.h"
#include "seccomon.h"

namespace insanity { namespace test {

class OCSPResponseContext
{
public:
  PLArenaPool* arena;
  
  pkix::ScopedCERTCertificate cert; 
  pkix::ScopedCERTCertificate issuerCert; 
  pkix::ScopedCERTCertificate signerCert; 
  uint8_t responseStatus; 
  

  
  
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

} } 

#endif 
