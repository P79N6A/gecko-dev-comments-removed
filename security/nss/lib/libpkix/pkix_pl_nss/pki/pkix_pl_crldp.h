









































#include "pkix_pl_common.h"

#ifndef _PKIX_PL_CRLDP_H
#define _PKIX_PL_CRLDP_H

#ifdef __cplusplus
extern "C" {
#endif





typedef struct pkix_pl_CrlDpStruct {
    
    const CRLDistributionPoint *nssdp;
    DistributionPointTypes distPointType;
    union {
	CERTGeneralName *fullName;
        


        CERTName *issuerName;
    } name;
    PKIX_Boolean isPartitionedByReasonCode;
} pkix_pl_CrlDp;


PKIX_Error *
pkix_pl_CrlDp_RegisterSelf(void *plContext);



PKIX_Error *
pkix_pl_CrlDp_Create(const CRLDistributionPoint *dp,
                     const CERTName *certIssuerName,
                     pkix_pl_CrlDp **pPkixDP,
                     void *plContext);
#endif
