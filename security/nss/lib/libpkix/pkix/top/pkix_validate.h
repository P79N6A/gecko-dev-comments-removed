










































#ifndef _PKIX_VALIDATE_H
#define _PKIX_VALIDATE_H
#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

PKIX_Error *
pkix_CheckChain(
        PKIX_List *certs,
        PKIX_UInt32 numCerts,
        PKIX_List *checkers,
        PKIX_List *revCheckers,
        PKIX_List *buildCheckedExtOIDs,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 *pCertCheckedIndex,
        PKIX_UInt32 *pCheckerIndex,
        PKIX_Boolean *pRevChecking,
        PKIX_UInt32 *pReasonCode,
        void **pNBIOContext,
        PKIX_PL_PublicKey **pFinalSubjPubKey,
        PKIX_PolicyNode **pPolicyTree,
        PKIX_VerifyNode **pVerifyTree,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
