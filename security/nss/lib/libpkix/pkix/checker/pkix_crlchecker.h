










































#ifndef _PKIX_CRLCHECKER_H
#define _PKIX_CRLCHECKER_H

#include "pkixt.h"
#include "pkix_revocationmethod.h"
#include "pkix_crlsel.h"

#ifdef __cplusplus
extern "C" {
#endif



PKIX_Error *
pkix_CrlChecker_CheckLocal(
        PKIX_PL_Cert *cert,
        PKIX_PL_Cert *issuer,
        PKIX_PL_Date *date,
        pkix_RevocationMethod *checkerObject,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 methodFlags,
        PKIX_RevocationStatus *pRevStatus,
        PKIX_UInt32 *reasonCode,
        void *plContext);

PKIX_Error *
pkix_CrlChecker_CheckExternal(
        PKIX_PL_Cert *cert,
        PKIX_PL_Cert *issuer,
        PKIX_PL_Date *date,
        pkix_RevocationMethod *checkerObject,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 methodFlags,
        PKIX_RevocationStatus *pRevStatus,
        PKIX_UInt32 *reasonCode,
        void **pNBIOContext,
        void *plContext);

PKIX_Error *
pkix_CrlChecker_Create(PKIX_RevocationMethodType methodType,
                       PKIX_UInt32 flags,
                       PKIX_UInt32 priority,
                       pkix_LocalRevocationCheckFn localRevChecker,
                       pkix_ExternalRevocationCheckFn externalRevChecker,
                       PKIX_List *certStores,
                       PKIX_PL_VerifyCallback crlVerifyFn,
                       pkix_RevocationMethod **pChecker,
                       void *plContext);

PKIX_Error *
pkix_CrlChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
