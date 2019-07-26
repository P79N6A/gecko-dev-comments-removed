









#ifndef _PKIX_PL_POLICYQUALIFIER_H
#define _PKIX_PL_POLICYQUALIFIER_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif













struct PKIX_PL_CertPolicyQualifierStruct {
        PKIX_PL_OID *policyQualifierId;
        PKIX_PL_ByteArray *qualifier;
};

PKIX_Error *
pkix_pl_CertPolicyQualifier_Create(
        PKIX_PL_OID *oid,
        PKIX_PL_ByteArray *qualifierArray,
        PKIX_PL_CertPolicyQualifier **pObject,
        void *plContext);

PKIX_Error *
pkix_pl_CertPolicyQualifier_RegisterSelf(
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
