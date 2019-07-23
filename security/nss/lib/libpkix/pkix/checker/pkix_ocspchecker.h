










































#ifndef _PKIX_OCSPCHECKER_H
#define _PKIX_OCSPCHECKER_H

#include "pkix_tools.h"
#include "pkix_revocationmethod.h"

#ifdef __cplusplus
extern "C" {
#endif



PKIX_Error *
pkix_OcspChecker_CheckLocal(
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
pkix_OcspChecker_CheckExternal(
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
pkix_OcspChecker_Create(PKIX_RevocationMethodType methodType,
                        PKIX_UInt32 flags,
                        PKIX_UInt32 priority,
                        pkix_LocalRevocationCheckFn localRevChecker,
                        pkix_ExternalRevocationCheckFn externalRevChecker,
                        PKIX_PL_VerifyCallback certVerifyFn,
                        pkix_RevocationMethod **pChecker,
                        void *plContext);



PKIX_Error *pkix_OcspChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
