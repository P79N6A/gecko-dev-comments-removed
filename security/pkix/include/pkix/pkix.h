























#ifndef mozilla_pkix__pkix_h
#define mozilla_pkix__pkix_h

#include "pkixtypes.h"
#include "prtime.h"

namespace mozilla { namespace pkix {




























































SECStatus BuildCertChain(TrustDomain& trustDomain,
                         const CERTCertificate* cert,
                         PRTime time,
                         EndEntityOrCA endEntityOrCA,
                         KeyUsage requiredKeyUsageIfPresent,
                         KeyPurposeId requiredEKUIfPresent,
                         const CertPolicyId& requiredPolicy,
             const SECItem* stapledOCSPResponse,
                  ScopedCERTCertList& results);


SECStatus VerifySignedData(const CERTSignedData* sd,
                           const SECItem& subjectPublicKeyInfo,
                           void* pkcs11PinArg);


SECItem* CreateEncodedOCSPRequest(PLArenaPool* arena,
                                  const CERTCertificate* cert,
                                  const CERTCertificate* issuerCert);








SECStatus VerifyEncodedOCSPResponse(TrustDomain& trustDomain,
                                    const CERTCertificate* cert,
                                    CERTCertificate* issuerCert,
                                    PRTime time,
                                    uint16_t maxLifetimeInDays,
                                    const SECItem* encodedResponse,
                  PRTime* thisUpdate,
                  PRTime* validThrough);

} } 

#endif 
