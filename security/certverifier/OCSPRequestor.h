





#ifndef mozilla_psm_OCSPRequestor_h
#define mozilla_psm_OCSPRequestor_h

#include "CertVerifier.h"
#include "secmodt.h"

namespace mozilla { namespace psm {


SECItem* DoOCSPRequest(PLArenaPool* arena, const char* url,
                       const SECItem* encodedRequest, PRIntervalTime timeout,
                       bool useGET);

} } 

#endif 
