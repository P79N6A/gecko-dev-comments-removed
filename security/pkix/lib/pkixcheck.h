























#ifndef mozilla_pkix__pkixcheck_h
#define mozilla_pkix__pkixcheck_h

#include "pkix/pkixtypes.h"
#include "pkixutil.h"
#include "certt.h"

namespace mozilla { namespace pkix {

Result CheckIssuerIndependentProperties(
          TrustDomain& trustDomain,
          const BackCert& cert,
          PRTime time,
          KeyUsage requiredKeyUsageIfPresent,
          KeyPurposeId requiredEKUIfPresent,
          const CertPolicyId& requiredPolicy,
          unsigned int subCACount,
           TrustLevel& trustLevel);

Result CheckNameConstraints(const SECItem& encodedNameConstraints,
                            const BackCert& firstChild,
                            KeyPurposeId requiredEKUIfPresent);

} } 

#endif 
