










































#ifndef _PKIX_EKUCHECKER_H
#define _PKIX_EKUCHECKER_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif



























PKIX_Error *
PKIX_EkuChecker_Create(
        PKIX_ProcessingParams *params,
        PKIX_CertChainChecker **ekuChecker,
        void *plContext);




























PKIX_Error *
pkix_EkuChecker_GetRequiredEku(
        PKIX_CertSelector *certSelector,
        PKIX_UInt32 *pRequiredExtKeyUsage,
        void *plContext);


PKIX_Error *pkix_EkuChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
