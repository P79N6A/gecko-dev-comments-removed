
















#ifndef insanity__pkixcheck_h
#define insanity__pkixcheck_h

#include "pkixutil.h"
#include "certt.h"

namespace insanity { namespace pkix {

Result CheckTimes(const CERTCertificate* cert, PRTime time);

Result CheckExtensions(BackCert& certExt,
                       EndEntityOrCA endEntityOrCA,
                       bool isTrustAnchor,
                       KeyUsages requiredKeyUsagesIfPresent,
                       SECOidTag requiredEKUIfPresent,
                       unsigned int depth);

Result CheckNameConstraints(BackCert& cert);

} } 

#endif 
