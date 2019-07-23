










































#ifndef _PKIX_EXPIRATIONCHECKER_H
#define _PKIX_EXPIRATIONCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

PKIX_Error *
pkix_ExpirationChecker_Initialize(
        PKIX_PL_Date *testDate,
        PKIX_CertChainChecker **pChecker,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
