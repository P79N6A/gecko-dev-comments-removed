























#ifndef mozilla_pkix__pkixcheck_h
#define mozilla_pkix__pkixcheck_h

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

} } 

#endif 
