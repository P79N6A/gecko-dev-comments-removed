











































#include "pkix_pl_nsscontext.h"

#define PKIX_DEFAULT_MAX_RESPONSE_LENGTH               64 * 1024
#define PKIX_DEFAULT_COMM_TIMEOUT_SECONDS              60

#define PKIX_DEFAULT_CRL_RELOAD_DELAY_SECONDS        6 * 24 * 60 * 60
#define PKIX_DEFAULT_BAD_CRL_RELOAD_DELAY_SECONDS    60 * 60







PKIX_Error *
PKIX_PL_NssContext_Create(
        PKIX_UInt32 certificateUsage,
        PKIX_Boolean useNssArena,
        void *wincx,
        void **pNssContext)
{
        PKIX_PL_NssContext *context = NULL;
        PRArenaPool *arena = NULL;
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "PKIX_PL_NssContext_Create");
        PKIX_NULLCHECK_ONE(pNssContext);

        PKIX_CHECK(PKIX_PL_Malloc
                   (sizeof(PKIX_PL_NssContext), (void **)&context, NULL),
                   PKIX_MALLOCFAILED);

        if (useNssArena == PKIX_TRUE) {
                PKIX_CONTEXT_DEBUG("\t\tCalling PORT_NewArena\n");
                arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        }
        
        context->arena = arena;
        context->certificateUsage = (SECCertificateUsage)certificateUsage;
        context->wincx = wincx;
        context->timeoutSeconds = PKIX_DEFAULT_COMM_TIMEOUT_SECONDS;
        context->maxResponseLength = PKIX_DEFAULT_MAX_RESPONSE_LENGTH;
        context->crlReloadDelay = PKIX_DEFAULT_CRL_RELOAD_DELAY_SECONDS;
        context->badDerCrlReloadDelay =
                             PKIX_DEFAULT_BAD_CRL_RELOAD_DELAY_SECONDS;
        *pNssContext = context;

cleanup:

        PKIX_RETURN(CONTEXT);
}






PKIX_Error *
PKIX_PL_NssContext_Destroy(
        void *nssContext)
{
        void *plContext = NULL;
        PKIX_PL_NssContext *context = NULL;

        PKIX_ENTER(CONTEXT, "PKIX_PL_NssContext_Destroy");
        PKIX_NULLCHECK_ONE(nssContext);

        context = (PKIX_PL_NssContext*)nssContext;

        if (context->arena != NULL) {
                PKIX_CONTEXT_DEBUG("\t\tCalling PORT_FreeArena\n");
                PORT_FreeArena(context->arena, PKIX_FALSE);
        }

        PKIX_PL_Free(nssContext, NULL);

        PKIX_RETURN(CONTEXT);
}





















PKIX_Error *
pkix_pl_NssContext_GetCertUsage(
        PKIX_PL_NssContext *nssContext,
        SECCertificateUsage *pCertUsage)
{
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "pkix_pl_NssContext_GetCertUsage");
        PKIX_NULLCHECK_TWO(nssContext, pCertUsage);

        *pCertUsage = nssContext->certificateUsage;

        PKIX_RETURN(CONTEXT);
}





















PKIX_Error *
pkix_pl_NssContext_SetCertUsage(
        SECCertificateUsage certUsage,
        PKIX_PL_NssContext *nssContext)
{
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "pkix_pl_NssContext_SetCertUsage");
        PKIX_NULLCHECK_ONE(nssContext);

        nssContext->certificateUsage = certUsage;

        PKIX_RETURN(CONTEXT);
}




















PKIX_Error *
pkix_pl_NssContext_GetWincx(
        PKIX_PL_NssContext *nssContext,
        void **pWincx)
{
        void *plContext = NULL;
        PKIX_PL_NssContext *context = NULL;

        PKIX_ENTER(CONTEXT, "pkix_pl_NssContext_GetWincx");
        PKIX_NULLCHECK_TWO(nssContext, pWincx);

        context = (PKIX_PL_NssContext *)nssContext;

        *pWincx = context->wincx;

        PKIX_RETURN(CONTEXT);
}




















PKIX_Error *
pkix_pl_NssContext_SetWincx(
        void *wincx,
        PKIX_PL_NssContext *nssContext)
{
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "pkix_pl_NssContext_SetWincx");
        PKIX_NULLCHECK_ONE(nssContext);

        nssContext->wincx = wincx;

        PKIX_RETURN(CONTEXT);
}









PKIX_Error *
PKIX_PL_NssContext_SetTimeout(PKIX_UInt32 timeout,
                              PKIX_PL_NssContext *nssContext)
{
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "PKIX_PL_NssContext_SetTimeout");
        PKIX_NULLCHECK_ONE(nssContext);

        nssContext->timeoutSeconds = timeout;

        PKIX_RETURN(CONTEXT);
}








PKIX_Error *
PKIX_PL_NssContext_SetMaxResponseLen(PKIX_UInt32 len,
                                     PKIX_PL_NssContext *nssContext)
{
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "PKIX_PL_NssContext_SetMaxResponseLen");
        PKIX_NULLCHECK_ONE(nssContext);

        nssContext->maxResponseLength = len;

        PKIX_RETURN(CONTEXT);
}









PKIX_Error *
PKIX_PL_NssContext_SetCrlReloadDelay(PKIX_UInt32 delay,
                                     PKIX_PL_NssContext *nssContext)
{
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "PKIX_PL_NssContext_SetCrlReloadDelay");
        PKIX_NULLCHECK_ONE(nssContext);

        nssContext->crlReloadDelay = delay;

        PKIX_RETURN(CONTEXT);
}









PKIX_Error *
PKIX_PL_NssContext_SetBadDerCrlReloadDelay(PKIX_UInt32 delay,
                                             PKIX_PL_NssContext *nssContext)
{
        void *plContext = NULL;

        PKIX_ENTER(CONTEXT, "PKIX_PL_NssContext_SetBadDerCrlReloadDelay");
        PKIX_NULLCHECK_ONE(nssContext);

        nssContext->badDerCrlReloadDelay = delay;

        PKIX_RETURN(CONTEXT);
}
