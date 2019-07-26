









#ifndef _PKIX_VALIDATERESULT_H
#define _PKIX_VALIDATERESULT_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_ValidateResultStruct {
        PKIX_PL_PublicKey *pubKey;
        PKIX_TrustAnchor *anchor;
        PKIX_PolicyNode *policyTree;
};



PKIX_Error *
pkix_ValidateResult_Create(
        PKIX_PL_PublicKey *pubKey,
        PKIX_TrustAnchor *anchor,
        PKIX_PolicyNode *policyTree,
        PKIX_ValidateResult **pResult,
        void *plContext);

PKIX_Error *pkix_ValidateResult_RegisterSelf(void *plContext);


#ifdef __cplusplus
}
#endif

#endif
