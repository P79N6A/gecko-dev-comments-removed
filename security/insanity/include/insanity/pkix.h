
















#ifndef insanity_pkix__pkix_h
#define insanity_pkix__pkix_h

#include "certt.h"
#include "seccomon.h"

namespace insanity { namespace pkix {



SECStatus VerifySignedData(const CERTSignedData* sd,
                           const CERTCertificate* cert,
                           void* pkcs11PinArg);

} } 

#endif 
