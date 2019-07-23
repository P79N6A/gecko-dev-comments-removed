










































#ifndef _PKIX_SIGNATURECHECKER_H
#define _PKIX_SIGNATURECHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pkix_SignatureCheckerState pkix_SignatureCheckerState;

struct pkix_SignatureCheckerState {
        PKIX_Boolean prevCertCertSign;
        PKIX_UInt32 certsRemaining;
        PKIX_PL_PublicKey *prevPublicKey; 
        PKIX_List *prevPublicKeyList; 
        PKIX_PL_OID *keyUsageOID;
};

PKIX_Error *
pkix_SignatureChecker_Initialize(
        PKIX_PL_PublicKey *trustedPubKey,
        PKIX_UInt32 certsRemaining,
        PKIX_CertChainChecker **pChecker,
        void *plContext);

PKIX_Error *
pkix_SignatureCheckerState_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
