







#ifndef _PKIX_CHECKER_H
#define _PKIX_CHECKER_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif








































































































typedef PKIX_Error *
(*PKIX_CertChainChecker_CheckCallback)(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,  
        void **pNBIOContext,
        void *plContext);



























































PKIX_Error *
PKIX_CertChainChecker_Create(
    PKIX_CertChainChecker_CheckCallback callback,
    PKIX_Boolean forwardCheckingSupported,
    PKIX_Boolean forwardDirectionExpected,
    PKIX_List *extensions,  
    PKIX_PL_Object *initialState,
    PKIX_CertChainChecker **pChecker,
    void *plContext);























PKIX_Error *
PKIX_CertChainChecker_GetCheckCallback(
        PKIX_CertChainChecker *checker,
        PKIX_CertChainChecker_CheckCallback *pCallback,
        void *plContext);






























PKIX_Error *
PKIX_CertChainChecker_IsForwardCheckingSupported(
        PKIX_CertChainChecker *checker,
        PKIX_Boolean *pForwardCheckingSupported,
        void *plContext);































PKIX_Error *
PKIX_CertChainChecker_IsForwardDirectionExpected(
        PKIX_CertChainChecker *checker,
        PKIX_Boolean *pForwardDirectionExpected,
        void *plContext);






























PKIX_Error *
PKIX_CertChainChecker_GetSupportedExtensions(
        PKIX_CertChainChecker *checker,
        PKIX_List **pExtensions, 
        void *plContext);

























PKIX_Error *
PKIX_CertChainChecker_GetCertChainCheckerState(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Object **pCertChainCheckerState,
        void *plContext);
























PKIX_Error *
PKIX_CertChainChecker_SetCertChainCheckerState(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Object *certChainCheckerState,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
