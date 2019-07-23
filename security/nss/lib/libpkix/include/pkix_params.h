









































#ifndef _PKIX_PARAMS_H
#define _PKIX_PARAMS_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif






































































PKIX_Error *
PKIX_ProcessingParams_Create(
        PKIX_ProcessingParams **pParams,
        void *plContext);




























PKIX_Error *
PKIX_ProcessingParams_GetCertChainCheckers(
        PKIX_ProcessingParams *params,
        PKIX_List **pCheckers, 
        void *plContext);




























PKIX_Error *
PKIX_ProcessingParams_SetCertChainCheckers(
        PKIX_ProcessingParams *params,
        PKIX_List *checkers,  
        void *plContext);

























PKIX_Error *
PKIX_ProcessingParams_AddCertChainChecker(
        PKIX_ProcessingParams *params,
        PKIX_CertChainChecker *checker,
        void *plContext);




























PKIX_Error *
PKIX_ProcessingParams_GetRevocationChecker(
        PKIX_ProcessingParams *params,
        PKIX_RevocationChecker **pChecker,
        void *plContext);



























PKIX_Error *
PKIX_ProcessingParams_SetRevocationChecker(
        PKIX_ProcessingParams *params,
        PKIX_RevocationChecker *revChecker,
        void *plContext);




























PKIX_Error *
PKIX_ProcessingParams_GetCertStores(
        PKIX_ProcessingParams *params,
        PKIX_List **pStores,  
        void *plContext);




























PKIX_Error *
PKIX_ProcessingParams_SetCertStores(
        PKIX_ProcessingParams *params,
        PKIX_List *stores,  
        void *plContext);

























PKIX_Error *
PKIX_ProcessingParams_AddCertStore(
        PKIX_ProcessingParams *params,
        PKIX_CertStore *store,
        void *plContext);



























PKIX_Error *
PKIX_ProcessingParams_GetDate(
        PKIX_ProcessingParams *params,
        PKIX_PL_Date **pDate,
        void *plContext);

























PKIX_Error *
PKIX_ProcessingParams_SetDate(
        PKIX_ProcessingParams *params,
        PKIX_PL_Date *date,
        void *plContext);





























PKIX_Error *
PKIX_ProcessingParams_GetInitialPolicies(
        PKIX_ProcessingParams *params,
        PKIX_List **pInitPolicies,    
        void *plContext);

































PKIX_Error *
PKIX_ProcessingParams_SetInitialPolicies(
        PKIX_ProcessingParams *params,
        PKIX_List *initPolicies,    
        void *plContext);

























PKIX_Error *
PKIX_ProcessingParams_GetPolicyQualifiersRejected(
        PKIX_ProcessingParams *params,
        PKIX_Boolean *pRejected,
        void *plContext);























PKIX_Error *
PKIX_ProcessingParams_SetPolicyQualifiersRejected(
        PKIX_ProcessingParams *params,
        PKIX_Boolean rejected,
        void *plContext);



























PKIX_Error *
PKIX_ProcessingParams_GetTargetCertConstraints(
        PKIX_ProcessingParams *params,
        PKIX_CertSelector **pConstraints,
        void *plContext);


























PKIX_Error *
PKIX_ProcessingParams_SetTargetCertConstraints(
        PKIX_ProcessingParams *params,
        PKIX_CertSelector *constraints,
        void *plContext);


























PKIX_Error *
PKIX_ProcessingParams_GetTrustAnchors(
        PKIX_ProcessingParams *params,
        PKIX_List **pAnchors,  
        void *plContext);























PKIX_Error *
PKIX_ProcessingParams_SetTrustAnchors(
        PKIX_ProcessingParams *params,
        PKIX_List *pAnchors,  
        void *plContext);


























PKIX_Error *
PKIX_ProcessingParams_GetUseAIAForCertFetching(
        PKIX_ProcessingParams *params,
        PKIX_Boolean *pUseAIA,  
        void *plContext);






















PKIX_Error *
PKIX_ProcessingParams_SetUseAIAForCertFetching(
        PKIX_ProcessingParams *params,
        PKIX_Boolean useAIA,  
        void *plContext);


























PKIX_Error *
PKIX_ProcessingParams_SetQualifyTargetCert(
        PKIX_ProcessingParams *params,
        PKIX_Boolean qualifyTargetCert,
        void *plContext);


























PKIX_Error *
PKIX_ProcessingParams_GetHintCerts(
        PKIX_ProcessingParams *params,
        PKIX_List **pHintCerts,
        void *plContext);

























PKIX_Error *
PKIX_ProcessingParams_SetHintCerts(
        PKIX_ProcessingParams *params,
        PKIX_List *hintCerts,
        void *plContext);




























PKIX_Error *
PKIX_ProcessingParams_GetResourceLimits(
        PKIX_ProcessingParams *params,
        PKIX_ResourceLimits **pResourceLimits,
        void *plContext);



























PKIX_Error *
PKIX_ProcessingParams_SetResourceLimits(
        PKIX_ProcessingParams *params,
        PKIX_ResourceLimits *resourceLimits,
        void *plContext);
























PKIX_Error *
PKIX_ProcessingParams_IsAnyPolicyInhibited(
        PKIX_ProcessingParams *params,
        PKIX_Boolean *pInhibited,
        void *plContext);























PKIX_Error *
PKIX_ProcessingParams_SetAnyPolicyInhibited(
        PKIX_ProcessingParams *params,
        PKIX_Boolean inhibited,
        void *plContext);

























PKIX_Error *
PKIX_ProcessingParams_IsExplicitPolicyRequired(
        PKIX_ProcessingParams *params,
        PKIX_Boolean *pRequired,
        void *plContext);























PKIX_Error *
PKIX_ProcessingParams_SetExplicitPolicyRequired(
        PKIX_ProcessingParams *params,
        PKIX_Boolean required,
        void *plContext);
























PKIX_Error *
PKIX_ProcessingParams_IsPolicyMappingInhibited(
        PKIX_ProcessingParams *params,
        PKIX_Boolean *pInhibited,
        void *plContext);























PKIX_Error *
PKIX_ProcessingParams_SetPolicyMappingInhibited(
        PKIX_ProcessingParams *params,
        PKIX_Boolean inhibited,
        void *plContext);






























PKIX_Error *
PKIX_ValidateParams_Create(
        PKIX_ProcessingParams *procParams,
        PKIX_List *chain,
        PKIX_ValidateParams **pParams,
        void *plContext);


























PKIX_Error *
PKIX_ValidateParams_GetProcessingParams(
        PKIX_ValidateParams *valParams,
        PKIX_ProcessingParams **pProcParams,
        void *plContext);

























PKIX_Error *
PKIX_ValidateParams_GetCertChain(
        PKIX_ValidateParams *valParams,
        PKIX_List **pChain,
        void *plContext);































PKIX_Error *
PKIX_TrustAnchor_CreateWithCert(
        PKIX_PL_Cert *cert,
        PKIX_TrustAnchor **pAnchor,
        void *plContext);































PKIX_Error *
PKIX_TrustAnchor_CreateWithNameKeyPair(
        PKIX_PL_X500Name *name,
        PKIX_PL_PublicKey *pubKey,
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_TrustAnchor **pAnchor,
        void *plContext);























PKIX_Error *
PKIX_TrustAnchor_GetTrustedCert(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_Cert **pCert,
        void *plContext);























PKIX_Error *
PKIX_TrustAnchor_GetCAName(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_X500Name **pCAName,
        void *plContext);
























PKIX_Error *
PKIX_TrustAnchor_GetCAPublicKey(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_PublicKey **pPubKey,
        void *plContext);

























PKIX_Error *
PKIX_TrustAnchor_GetNameConstraints(
        PKIX_TrustAnchor *anchor,
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext);





























PKIX_Error *
PKIX_ResourceLimits_Create(
        PKIX_ResourceLimits **pResourceLimits,
        void *plContext);




























PKIX_Error *
PKIX_ResourceLimits_GetMaxTime(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 *pMaxTime,
        void *plContext);




























PKIX_Error *
PKIX_ResourceLimits_SetMaxTime(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 maxTime,
        void *plContext);































PKIX_Error *
PKIX_ResourceLimits_GetMaxFanout(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 *pMaxFanout,
        void *plContext);































PKIX_Error *
PKIX_ResourceLimits_SetMaxFanout(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 maxFanout,
        void *plContext);































PKIX_Error *
PKIX_ResourceLimits_GetMaxDepth(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 *pMaxDepth,
        void *plContext);































PKIX_Error *
PKIX_ResourceLimits_SetMaxDepth(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 maxDepth,
        void *plContext);































PKIX_Error *
PKIX_ResourceLimits_GetMaxNumberOfCerts(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 *pMaxNumber,
        void *plContext);






























PKIX_Error *
PKIX_ResourceLimits_SetMaxNumberOfCerts(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 maxNumber,
        void *plContext);































PKIX_Error *
PKIX_ResourceLimits_GetMaxNumberOfCRLs(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 *pMaxNumber,
        void *plContext);






























PKIX_Error *
PKIX_ResourceLimits_SetMaxNumberOfCRLs(
        PKIX_ResourceLimits *resourceLimits,
        PKIX_UInt32 maxNumber,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
