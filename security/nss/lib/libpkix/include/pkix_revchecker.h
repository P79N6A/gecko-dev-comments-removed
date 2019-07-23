









































#ifndef _PKIX_REVCHECKER_H
#define _PKIX_REVCHECKER_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif




















































































































typedef PKIX_Error *
(*PKIX_RevocationChecker_RevCallback)(
        PKIX_PL_Object *revCheckerContext,
        PKIX_PL_Cert *cert,
        PKIX_ProcessingParams *procParams,
        void **pNBIOContext,
        PKIX_UInt32 *pResultCode,
        void *plContext);

























PKIX_Error *
PKIX_RevocationChecker_Create(
        PKIX_RevocationChecker_RevCallback callback,
        PKIX_PL_Object *revCheckerContext,
        PKIX_RevocationChecker **pRevChecker,
        void *plContext);























PKIX_Error *
PKIX_RevocationChecker_GetRevCallback(
        PKIX_RevocationChecker *revChecker,
        PKIX_RevocationChecker_RevCallback *pCallback,
        void *plContext);
























PKIX_Error *
PKIX_RevocationChecker_GetRevCheckerContext(
        PKIX_RevocationChecker *revChecker,
        PKIX_PL_Object **pRevCheckerContext,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
