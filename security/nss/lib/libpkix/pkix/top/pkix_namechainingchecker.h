










































#ifndef _PKIX_NAMECHAININGCHECKER_H
#define _PKIX_NAMECHAININGCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

PKIX_Error *
pkix_NameChainingChecker_Initialize(
        PKIX_PL_X500Name *trustedCAName,
        PKIX_CertChainChecker **pChecker,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
