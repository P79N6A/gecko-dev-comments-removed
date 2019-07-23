










































#ifndef _PKIX_CRLSEL_H
#define _PKIX_CRLSEL_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif























































































typedef PKIX_Error *
(*PKIX_CRLSelector_MatchCallback)(
        PKIX_CRLSelector *selector,
        PKIX_PL_CRL *crl,
        PKIX_Boolean *pMatch,
        void *plContext);































PKIX_Error *
PKIX_CRLSelector_Create(
        PKIX_PL_Cert *issuer,
        PKIX_List *crlDpList,
        PKIX_PL_Date *date,
        PKIX_CRLSelector **pSelector,
        void *plContext);























PKIX_Error *
PKIX_CRLSelector_GetMatchCallback(
        PKIX_CRLSelector *selector,
        PKIX_CRLSelector_MatchCallback *pCallback,
        void *plContext);























PKIX_Error *
PKIX_CRLSelector_GetCRLSelectorContext(
        PKIX_CRLSelector *selector,
        void **pCRLSelectorContext,
        void *plContext);


























PKIX_Error *
PKIX_CRLSelector_GetCommonCRLSelectorParams(
        PKIX_CRLSelector *selector,
        PKIX_ComCRLSelParams **pCommonCRLSelectorParams,
        void *plContext);
























PKIX_Error *
PKIX_CRLSelector_SetCommonCRLSelectorParams(
        PKIX_CRLSelector *selector,
        PKIX_ComCRLSelParams *commonCRLSelectorParams,
        void *plContext);




























PKIX_Error *
PKIX_ComCRLSelParams_Create(
        PKIX_ComCRLSelParams **pParams,
        void *plContext);
































PKIX_Error *
PKIX_ComCRLSelParams_GetIssuerNames(
        PKIX_ComCRLSelParams *params,
        PKIX_List **pNames,  
        void *plContext);


























PKIX_Error *
PKIX_ComCRLSelParams_SetIssuerNames(
        PKIX_ComCRLSelParams *params,
        PKIX_List *names,   
        void *plContext);


























PKIX_Error *
PKIX_ComCRLSelParams_AddIssuerName(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_X500Name *name,
        void *plContext);




























PKIX_Error *
PKIX_ComCRLSelParams_GetCertificateChecking(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_Cert **pCert,
        void *plContext);


























PKIX_Error *
PKIX_ComCRLSelParams_SetCertificateChecking(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_Cert *cert,
        void *plContext);
































PKIX_Error *
PKIX_ComCRLSelParams_GetDateAndTime(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_Date **pDate,
        void *plContext);




























PKIX_Error *
PKIX_ComCRLSelParams_SetDateAndTime(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_Date *date,
        void *plContext);



























PKIX_Error *
PKIX_ComCRLSelParams_GetNISTPolicyEnabled(
        PKIX_ComCRLSelParams *params,
        PKIX_Boolean *pEnabled,
        void *plContext);


























PKIX_Error *
PKIX_ComCRLSelParams_SetNISTPolicyEnabled(
        PKIX_ComCRLSelParams *params,
        PKIX_Boolean enabled,
        void *plContext);






























PKIX_Error *
PKIX_ComCRLSelParams_GetMaxCRLNumber(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_BigInt **pNumber,
        void *plContext);


























PKIX_Error *
PKIX_ComCRLSelParams_SetMaxCRLNumber(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_BigInt *number,
        void *plContext);






























PKIX_Error *
PKIX_ComCRLSelParams_GetMinCRLNumber(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_BigInt **pNumber,
        void *plContext);


























PKIX_Error *
PKIX_ComCRLSelParams_SetMinCRLNumber(
        PKIX_ComCRLSelParams *params,
        PKIX_PL_BigInt *number,
        void *plContext);























PKIX_Error*
PKIX_ComCRLSelParams_SetCrlDp(
         PKIX_ComCRLSelParams *params,
         PKIX_List *crldpList,
         void *plContext);

#ifdef __cplusplus
}
#endif

#endif
