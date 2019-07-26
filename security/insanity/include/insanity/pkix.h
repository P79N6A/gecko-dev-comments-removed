
















#ifndef insanity_pkix__pkix_h
#define insanity_pkix__pkix_h

#include "pkixtypes.h"
#include "prtime.h"

namespace insanity { namespace pkix {

SECStatus BuildCertChain(TrustDomain& trustDomain,
                         CERTCertificate* cert,
                         PRTime time,
                  ScopedCERTCertList& results);



SECStatus VerifySignedData(const CERTSignedData* sd,
                           const CERTCertificate* cert,
                           void* pkcs11PinArg);

} } 

#endif 
