























#ifndef mozilla_pkix__pkix_h
#define mozilla_pkix__pkix_h

#include "pkixtypes.h"

namespace mozilla { namespace pkix {




























































Result BuildCertChain(TrustDomain& trustDomain, InputBuffer cert,
                      PRTime time, EndEntityOrCA endEntityOrCA,
                      KeyUsage requiredKeyUsageIfPresent,
                      KeyPurposeId requiredEKUIfPresent,
                      const CertPolicyId& requiredPolicy,
                       const InputBuffer* stapledOCSPResponse);

static const size_t OCSP_REQUEST_MAX_LENGTH = 127;
Result CreateEncodedOCSPRequest(TrustDomain& trustDomain,
                                const struct CertID& certID,
                                 uint8_t (&out)[OCSP_REQUEST_MAX_LENGTH],
                                 size_t& outLen);













Result VerifyEncodedOCSPResponse(TrustDomain& trustDomain,
                                 const CertID& certID, PRTime time,
                                 uint16_t maxLifetimeInDays,
                                 InputBuffer encodedResponse,
                        bool& expired,
               PRTime* thisUpdate = nullptr,
               PRTime* validThrough = nullptr);

} } 

#endif 
