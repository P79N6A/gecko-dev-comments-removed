









#ifndef _PKIX_PL_NAMECONSTRAINTS_H
#define _PKIX_PL_NAMECONSTRAINTS_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_CertNameConstraintsStruct {
        PLArenaPool *arena;
        CERTNameConstraints **nssNameConstraintsList;
        PKIX_UInt32 numNssNameConstraints;
        PKIX_List *permittedList; 
        PKIX_List *excludedList; 
};



PKIX_Error *pkix_pl_CertNameConstraints_RegisterSelf(void *plContext);

PKIX_Error *pkix_pl_CertNameConstraints_Create(
        CERTCertificate *nssCert,
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext);

PKIX_Error *
pkix_pl_CertNameConstraints_CreateWithNames(
        PKIX_List *names, 
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext);

PKIX_Error *
pkix_pl_CertNameConstraints_CheckNameSpaceNssNames(
        CERTGeneralName *nssSubjectNames,
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_Boolean *pCheckPass,
        void *plContext);

PKIX_Error *
pkix_pl_CertNameConstraints_CheckNameSpacePkixNames(
        PKIX_List *nameList,
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_Boolean *pCheckPass,
        void *plContext);


PKIX_Error *pkix_pl_CertNameConstraints_Merge(
        PKIX_PL_CertNameConstraints *firstNC,
        PKIX_PL_CertNameConstraints *secondNC,
        PKIX_PL_CertNameConstraints **pMergedNC,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
