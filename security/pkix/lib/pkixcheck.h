























#ifndef mozilla_pkix__pkixcheck_h
#define mozilla_pkix__pkixcheck_h

#include "pkix/pkixtypes.h"
#include "pkixutil.h"
#include "certt.h"

namespace mozilla { namespace pkix {

Result CheckIssuerIndependentProperties(
          TrustDomain& trustDomain,
          BackCert& cert,
          PRTime time,
          EndEntityOrCA endEntityOrCA,
          KeyUsage requiredKeyUsageIfPresent,
          KeyPurposeId requiredEKUIfPresent,
          const CertPolicyId& requiredPolicy,
          unsigned int subCACount,
           TrustLevel* trustLevel = nullptr);

Result CheckNameConstraints(BackCert& cert);

} } 

#endif 
