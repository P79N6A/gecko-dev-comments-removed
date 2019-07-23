









































#ifndef _PKIX_REVCHECKER_H
#define _PKIX_REVCHECKER_H

#include "pkixt.h"
#include "pkix_pl_pki.h"

#ifdef __cplusplus
extern "C" {
#endif











































































PKIX_Error *
PKIX_RevocationChecker_Create(
    PKIX_UInt32 leafMethodListFlags,
    PKIX_UInt32 chainMethodListFlags,
    PKIX_RevocationChecker **pChecker,
    void *plContext);








































PKIX_Error *
PKIX_RevocationChecker_CreateAndAddMethod(
    PKIX_RevocationChecker *revChecker,
    PKIX_ProcessingParams *params,
    PKIX_RevocationMethodType methodType,
    PKIX_UInt32 methodFlags,
    PKIX_UInt32 mathodPriority,
    PKIX_PL_VerifyCallback verificationFn,
    PKIX_Boolean isLeafMethod,
    void *plContext);



















































PKIX_Error *
PKIX_RevocationChecker_Check(PKIX_PL_Cert *cert,
                             PKIX_PL_Cert *issuer,
                             PKIX_RevocationChecker *revChecker,
                             PKIX_ProcessingParams *procParams,
                             PKIX_Boolean chainVerificationState,
                             PKIX_Boolean testingLeafCert,
                             PKIX_RevocationStatus *revStatus,
                             PKIX_UInt32 *pReasonCode,
                             void **pNbioContext,
                             void *plContext);
    
#ifdef __cplusplus
}
#endif

#endif
