










































#ifndef _PKIX_PL_EKUCHECKER_H
#define _PKIX_PL_EKUCHECKER_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pkix_pl_EkuChecker pkix_pl_EkuChecker;

struct pkix_pl_EkuChecker {
        PKIX_UInt32 requiredExtKeyUsage;
        PKIX_PL_OID *ekuOID;
};


PKIX_Error *pkix_pl_EkuChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
