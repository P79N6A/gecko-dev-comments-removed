









#ifndef _PKIX_PL_PK11CERTSTORE_H
#define _PKIX_PL_PK11CERTSTORE_H

#include "pkix_pl_common.h"
#include "certi.h"

#ifdef __cplusplus
extern "C" {
#endif


PKIX_Error *
PKIX_PL_Pk11CertStore_Create(
        PKIX_CertStore **pCertStore,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
