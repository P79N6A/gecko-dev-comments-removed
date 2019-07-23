










































#ifndef _PKIX_PL_OCSPREQUEST_H
#define _PKIX_PL_OCSPREQUEST_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_OcspRequestStruct{
        PKIX_PL_Cert *cert;
        PKIX_PL_Date *validity;
        PKIX_Boolean addServiceLocator;
        PKIX_PL_Cert *signerCert;
        CERTOCSPRequest *decoded;
        SECItem *encoded;
        char *location;
};



PKIX_Error *
pkix_pl_OcspRequest_Create(
        PKIX_PL_Cert *cert,
        PKIX_PL_OcspCertID *cid,
        PKIX_PL_Date *validity,
        PKIX_PL_Cert *signerCert,
        PKIX_UInt32 methodFlags,
        PKIX_Boolean *pURIFound,
        PKIX_PL_OcspRequest **pRequest,
        void *plContext);

PKIX_Error *
pkix_pl_OcspRequest_GetEncoded(
        PKIX_PL_OcspRequest *request,
        SECItem **pRequest,
        void *plContext);

PKIX_Error *
pkix_pl_OcspRequest_GetLocation(
        PKIX_PL_OcspRequest *request,
        char **pLocation,
        void *plContext);

PKIX_Error *pkix_pl_OcspRequest_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
