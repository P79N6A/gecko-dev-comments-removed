









#ifndef _PKIX_PL_COLCERTSTORE_H
#define _PKIX_PL_COLCERTSTORE_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_CollectionCertStoreContext {
        PKIX_PL_String *storeDir;
        PKIX_List *crlList;
        PKIX_List *certList;
};



PKIX_Error *pkix_pl_CollectionCertStoreContext_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
