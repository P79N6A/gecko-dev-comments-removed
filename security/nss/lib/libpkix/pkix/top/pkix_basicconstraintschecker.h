










































#ifndef _PKIX_BASICCONSTRAINTSCHECKER_H
#define _PKIX_BASICCONSTRAINTSCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pkix_BasicConstraintsCheckerStateStruct \
        pkix_BasicConstraintsCheckerState;

struct pkix_BasicConstraintsCheckerStateStruct{
        PKIX_PL_OID *basicConstraintsOID;
        PKIX_Int32 certsRemaining;
        PKIX_Int32 maxPathLength;
};

PKIX_Error *
pkix_BasicConstraintsChecker_Initialize(
        PKIX_UInt32 numCerts,
        PKIX_CertChainChecker **pChecker,
        void *plContext);

PKIX_Error *
pkix_BasicConstraintsCheckerState_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
