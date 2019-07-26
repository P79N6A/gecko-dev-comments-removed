









#ifndef _PKIX_PL_CERTPOLICYMAP_H
#define _PKIX_PL_CERTPOLICYMAP_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif










struct PKIX_PL_CertPolicyMapStruct {
        PKIX_PL_OID *issuerDomainPolicy;
        PKIX_PL_OID *subjectDomainPolicy;
};

PKIX_Error *
pkix_pl_CertPolicyMap_Create(
        PKIX_PL_OID *issuerDomainPolicy,
        PKIX_PL_OID *subjectDomainPolicy,
        PKIX_PL_CertPolicyMap **pObject,
        void *plContext);

PKIX_Error *
pkix_pl_CertPolicyMap_RegisterSelf(
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
