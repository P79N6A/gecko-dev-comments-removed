











































#ifndef _PKIX_PL_NSSCONTEXT_H
#define _PKIX_PL_NSSCONTEXT_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_NssContextStruct {
        SECCertificateUsage certificateUsage;
        PRArenaPool *arena;
        void *wincx;
        PKIX_UInt32 timeoutSeconds;
        PKIX_UInt32 maxResponseLength;
        PRTime crlReloadDelay;
        PRTime badDerCrlReloadDelay;
};

PKIX_Error *
pkix_pl_NssContext_GetCertUsage
        (PKIX_PL_NssContext *nssContext, SECCertificateUsage *pCertUsage);


PKIX_Error *
pkix_pl_NssContext_SetCertUsage
        (SECCertificateUsage certUsage, PKIX_PL_NssContext *nssContext);

PKIX_Error *
pkix_pl_NssContext_GetWincx(PKIX_PL_NssContext *nssContext, void **pWincx);


PKIX_Error *
pkix_pl_NssContext_SetWincx(void *wincx, PKIX_PL_NssContext *nssContext);

#ifdef __cplusplus
}
#endif

#endif
