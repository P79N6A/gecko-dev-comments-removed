








































#ifndef _PKIX_CERTSTORE_H
#define _PKIX_CERTSTORE_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif

















































































































typedef PKIX_Error *
(*PKIX_CertStore_CertCallback)(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *verifyNode,
        void **pNBIOContext,
        PKIX_List **pCerts,  
        void *plContext);
















































PKIX_Error *
PKIX_CertStore_CertContinue(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *verifyNode,
        void **pNBIOContext,
        PKIX_List **pCerts,  
        void *plContext);

typedef PKIX_Error *
(*PKIX_CertStore_CertContinueFunction)(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *verifyNode,
        void **pNBIOContext,
        PKIX_List **pCerts,  
        void *plContext);








































typedef PKIX_Error *
(*PKIX_CertStore_CRLCallback)(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrls,  
        void *plContext);


























typedef PKIX_Error *
(*PKIX_CertStore_ImportCrlCallback)(
        PKIX_CertStore *store,
        PKIX_List *crlList,
        void *plContext);




































typedef PKIX_Error *
(*PKIX_CertStore_CheckRevokationByCrlCallback)(
        PKIX_CertStore *store,
        PKIX_PL_Cert *cert,
        PKIX_PL_Cert *issuer,
        PKIX_PL_Date *date,
        PKIX_Boolean delayCrlSigCheck,
        PKIX_UInt32 *reasonCode,
        PKIX_RevocationStatus *revStatus,
        void *plContext);













































PKIX_Error *
PKIX_CertStore_CrlContinue(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrls,  
        void *plContext);

typedef PKIX_Error *
(*PKIX_CertStore_CrlContinueFunction)(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrls,  
        void *plContext);






























typedef PKIX_Error *
(*PKIX_CertStore_CheckTrustCallback)(
        PKIX_CertStore *store,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pTrusted,
        void *plContext);


















































PKIX_Error *
PKIX_CertStore_Create(
        PKIX_CertStore_CertCallback certCallback,
        PKIX_CertStore_CRLCallback crlCallback,
        PKIX_CertStore_CertContinueFunction certContinue,
        PKIX_CertStore_CrlContinueFunction crlContinue,
        PKIX_CertStore_CheckTrustCallback trustCallback,
        PKIX_CertStore_ImportCrlCallback importCrlCallback,
        PKIX_CertStore_CheckRevokationByCrlCallback checkRevByCrlCallback,
        PKIX_PL_Object *certStoreContext,
        PKIX_Boolean cachedFlag,
        PKIX_Boolean localFlag,
        PKIX_CertStore **pStore,
        void *plContext);






















PKIX_Error *
PKIX_CertStore_GetCertCallback(
        PKIX_CertStore *store,
        PKIX_CertStore_CertCallback *pCallback,
        void *plContext);






















PKIX_Error *
PKIX_CertStore_GetCRLCallback(
        PKIX_CertStore *store,
        PKIX_CertStore_CRLCallback *pCallback,
        void *plContext);






















PKIX_Error *
PKIX_CertStore_GetImportCrlCallback(
        PKIX_CertStore *store,
        PKIX_CertStore_ImportCrlCallback *pCallback,
        void *plContext);






















PKIX_Error *
PKIX_CertStore_GetCrlCheckerFn(
        PKIX_CertStore *store,
        PKIX_CertStore_CheckRevokationByCrlCallback *pCallback,
        void *plContext);






















PKIX_Error *
PKIX_CertStore_GetTrustCallback(
        PKIX_CertStore *store,
        PKIX_CertStore_CheckTrustCallback *pCallback,
        void *plContext);






















PKIX_Error *
PKIX_CertStore_GetCertStoreContext(
        PKIX_CertStore *store,
        PKIX_PL_Object **pCertStoreContext,
        void *plContext);





















PKIX_Error *
PKIX_CertStore_GetCertStoreCacheFlag(
        PKIX_CertStore *store,
        PKIX_Boolean *pCacheFlag,
        void *plContext);






















PKIX_Error *
PKIX_CertStore_GetLocalFlag(
        PKIX_CertStore *store,
        PKIX_Boolean *pLocalFlag,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
