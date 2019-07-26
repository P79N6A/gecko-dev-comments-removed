























#ifndef mozilla_pkix__pkix_h
#define mozilla_pkix__pkix_h

#include "pkixtypes.h"
#include "prtime.h"

namespace mozilla { namespace pkix {




























































SECStatus BuildCertChain(TrustDomain& trustDomain,
                         CERTCertificate* cert,
                         PRTime time,
                         EndEntityOrCA endEntityOrCA,
             KeyUsages requiredKeyUsagesIfPresent,
             SECOidTag requiredEKUIfPresent,
             SECOidTag requiredPolicy,
             const SECItem* stapledOCSPResponse,
                  ScopedCERTCertList& results);



SECStatus VerifySignedData(const CERTSignedData* sd,
                           const CERTCertificate* cert,
                           void* pkcs11PinArg);


SECItem* CreateEncodedOCSPRequest(PLArenaPool* arena,
                                  const CERTCertificate* cert,
                                  const CERTCertificate* issuerCert);








SECStatus VerifyEncodedOCSPResponse(TrustDomain& trustDomain,
                                    const CERTCertificate* cert,
                                    CERTCertificate* issuerCert,
                                    PRTime time,
                                    const SECItem* encodedResponse,
                  PRTime* thisUpdate,
                  PRTime* validThrough);

} } 

#endif 
