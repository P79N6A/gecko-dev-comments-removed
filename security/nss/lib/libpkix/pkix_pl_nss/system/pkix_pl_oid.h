










































#ifndef _PKIX_PL_OID_H
#define _PKIX_PL_OID_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_OIDStruct {
    SECItem derOid;
};



PKIX_Error *
pkix_pl_OID_RegisterSelf(void *plContext);

PKIX_Error *
pkix_pl_OID_GetCriticalExtensionOIDs(
        CERTCertExtension **extensions,
        PKIX_List **pOidsList,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
