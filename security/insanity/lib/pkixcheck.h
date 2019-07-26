
















#ifndef insanity__pkixcheck_h
#define insanity__pkixcheck_h

#include "pkixutil.h"
#include "certt.h"

namespace insanity { namespace pkix {

Result CheckIssuerIndependentProperties(
          TrustDomain& trustDomain,
          BackCert& cert,
          PRTime time,
          EndEntityOrCA endEntityOrCA,
          KeyUsages requiredKeyUsagesIfPresent,
          SECOidTag requiredEKUIfPresent,
          SECOidTag requiredPolicy,
          unsigned int subCACount,
           TrustDomain::TrustLevel* trustLevel = nullptr);

Result CheckNameConstraints(BackCert& cert);

} } 

#endif 
