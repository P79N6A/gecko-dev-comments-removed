









































#ifndef _PKIX_SAMPLEMODULES_H
#define _PKIX_SAMPLEMODULES_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif






























































PKIX_Error *
PKIX_PL_CollectionCertStore_Create(
        PKIX_PL_String *storeDir,
        PKIX_CertStore **pCertStore,
        void *plContext);






























PKIX_Error *
PKIX_PL_Pk11CertStore_Create(
        PKIX_CertStore **pPk11CertStore,
        void *plContext);



















































PKIX_Error *
PKIX_PL_LdapDefaultClient_Create(
        PRNetAddr *sockaddr,
        PRIntervalTime timeout,
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapDefaultClient **pClient,
        void *plContext);









































PKIX_Error *
PKIX_PL_LdapDefaultClient_CreateByName(
        char *hostname,
        PRIntervalTime timeout,
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapDefaultClient **pClient,
        void *plContext);






















PKIX_Error *
PKIX_PL_LdapCertStore_Create(
        PKIX_PL_LdapClient *client,
        PKIX_CertStore **pCertStore,
        void *plContext);

















































PKIX_Error *
PKIX_PL_NssContext_Create(
        PKIX_UInt32 certificateUsage,
        PKIX_Boolean useNssArena,
        void *wincx,
        void **pNssContext);



















PKIX_Error *
PKIX_PL_NssContext_Destroy(
        void *nssContext);


















PKIX_Error *
PKIX_PL_NssContext_SetTimeout(PKIX_UInt32 timeout, PKIX_PL_NssContext *nssContext);

















PKIX_Error *
PKIX_PL_NssContext_SetMaxResponseLen(PKIX_UInt32 len, PKIX_PL_NssContext *nssContext);




















PKIX_Error *
PKIX_PL_NssContext_SetCrlReloadDelay(PKIX_UInt32 delaySeconds,
                                       PKIX_PL_NssContext *nssContext);




















PKIX_Error *
PKIX_PL_NssContext_SetBadDerCrlReloadDelay(PKIX_UInt32 delaySeconds,
                                           PKIX_PL_NssContext *nssContext);
#ifdef __cplusplus
}
#endif

#endif
