























#ifndef mozilla_pkix_pkixcheck_h
#define mozilla_pkix_pkixcheck_h

#include "pkix/pkixtypes.h"

namespace mozilla { namespace pkix {

class BackCert;

Result CheckIssuerIndependentProperties(
          TrustDomain& trustDomain,
          const BackCert& cert,
          Time time,
          KeyUsage requiredKeyUsageIfPresent,
          KeyPurposeId requiredEKUIfPresent,
          const CertPolicyId& requiredPolicy,
          unsigned int subCACount,
           TrustLevel& trustLevel);

Result CheckNameConstraints(Input encodedNameConstraints,
                            const BackCert& firstChild,
                            KeyPurposeId requiredEKUIfPresent);

Result CheckValidity(Input encodedValidity, Time time,
                      Time* notBeforeOut = nullptr,
                      Time* notAfterOut = nullptr);

} } 

#endif 
