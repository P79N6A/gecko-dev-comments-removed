









#ifndef _PKIX_PL_BASICCONSTRAINTS_H
#define _PKIX_PL_BASICCONSTRAINTS_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif








struct PKIX_PL_CertBasicConstraintsStruct {
        PKIX_Boolean isCA;
        PKIX_Int32 pathLen;
};

PKIX_Error *
pkix_pl_CertBasicConstraints_Create(
        PKIX_Boolean isCA,
        PKIX_Int32 pathLen,
        PKIX_PL_CertBasicConstraints **object,
        void *plContext);

PKIX_Error *
pkix_pl_CertBasicConstraints_RegisterSelf(
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
