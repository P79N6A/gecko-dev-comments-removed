









#ifndef _PKIX_PL_POLICYINFO_H
#define _PKIX_PL_POLICYINFO_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif











struct PKIX_PL_CertPolicyInfoStruct {
        PKIX_PL_OID *cpID;
        PKIX_List *policyQualifiers; 
};

PKIX_Error *
pkix_pl_CertPolicyInfo_Create(
        PKIX_PL_OID *oid,
        PKIX_List *qualifiers,
        PKIX_PL_CertPolicyInfo **pObject,
        void *plContext);

PKIX_Error *
pkix_pl_CertPolicyInfo_RegisterSelf(
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
