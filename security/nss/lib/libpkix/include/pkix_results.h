








#ifndef _PKIX_RESULTS_H
#define _PKIX_RESULTS_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif
























































PKIX_Error *
PKIX_ValidateResult_GetPolicyTree(
        PKIX_ValidateResult *result,
        PKIX_PolicyNode **pPolicyTree,
        void *plContext);
























PKIX_Error *
PKIX_ValidateResult_GetPublicKey(
        PKIX_ValidateResult *result,
        PKIX_PL_PublicKey **pPublicKey,
        void *plContext);
























PKIX_Error *
PKIX_ValidateResult_GetTrustAnchor(
        PKIX_ValidateResult *result,
        PKIX_TrustAnchor **pTrustAnchor,
        void *plContext);































PKIX_Error *
PKIX_BuildResult_GetValidateResult(
        PKIX_BuildResult *result,
        PKIX_ValidateResult **pResult,
        void *plContext);
























PKIX_Error *
PKIX_BuildResult_GetCertChain(
        PKIX_BuildResult *result,
        PKIX_List **pChain,
        void *plContext);






































PKIX_Error *
PKIX_PolicyNode_GetChildren(
        PKIX_PolicyNode *node,
        PKIX_List **pChildren,  
        void *plContext);
























PKIX_Error *
PKIX_PolicyNode_GetParent(
        PKIX_PolicyNode *node,
        PKIX_PolicyNode **pParent,
        void *plContext);























PKIX_Error *
PKIX_PolicyNode_GetValidPolicy(
        PKIX_PolicyNode *node,
        PKIX_PL_OID **pValidPolicy,
        void *plContext);



























PKIX_Error *
PKIX_PolicyNode_GetPolicyQualifiers(
        PKIX_PolicyNode *node,
        PKIX_List **pQualifiers,  
        void *plContext);

























PKIX_Error *
PKIX_PolicyNode_GetExpectedPolicies(
        PKIX_PolicyNode *node,
        PKIX_List **pExpPolicies,  
        void *plContext);























PKIX_Error *
PKIX_PolicyNode_IsCritical(
        PKIX_PolicyNode *node,
        PKIX_Boolean *pCritical,
        void *plContext);























PKIX_Error *
PKIX_PolicyNode_GetDepth(
        PKIX_PolicyNode *node,
        PKIX_UInt32 *pDepth,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
