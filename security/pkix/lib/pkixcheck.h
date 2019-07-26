























#ifndef mozilla_pkix__pkixcheck_h
#define mozilla_pkix__pkixcheck_h

#include "pkixutil.h"
#include "certt.h"

namespace mozilla { namespace pkix {

Result CheckIssuerIndependentProperties(
          TrustDomain& trustDomain,
          BackCert& cert,
          PRTime time,
          EndEntityOrCA endEntityOrCA,
          KeyUsages requiredKeyUsagesIfPresent,
          SECOidTag requiredEKUIfPresent,
          SECOidTag requiredPolicy,
          unsigned int subCACount,
           TrustLevel* trustLevel = nullptr);

Result CheckNameConstraints(BackCert& cert);

} } 

#endif 
