










































#ifndef _PKIX_NAMECONSTRAINTSCHECKER_H
#define _PKIX_NAMECONSTRAINTSCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pkix_NameConstraintsCheckerState \
        pkix_NameConstraintsCheckerState;

struct pkix_NameConstraintsCheckerState {
        PKIX_PL_CertNameConstraints *nameConstraints;
        PKIX_PL_OID *nameConstraintsOID;
        PKIX_UInt32 certsRemaining;
};

PKIX_Error *
pkix_NameConstraintsChecker_Initialize(
        PKIX_PL_CertNameConstraints *trustedNC,
        PKIX_UInt32 numCerts,
        PKIX_CertChainChecker **pChecker,
        void *plContext);

PKIX_Error *
pkix_NameConstraintsCheckerState_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
