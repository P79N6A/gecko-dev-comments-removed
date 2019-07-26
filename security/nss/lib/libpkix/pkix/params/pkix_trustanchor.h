









#ifndef _PKIX_TRUSTANCHOR_H
#define _PKIX_TRUSTANCHOR_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_TrustAnchorStruct {
        PKIX_PL_Cert *trustedCert;
        PKIX_PL_X500Name *caName;
        PKIX_PL_PublicKey *caPubKey;
        PKIX_PL_CertNameConstraints *nameConstraints;
};



PKIX_Error *pkix_TrustAnchor_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
