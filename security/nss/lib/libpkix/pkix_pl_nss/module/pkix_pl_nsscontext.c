











































#include "pkix_pl_nsscontext.h"







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
