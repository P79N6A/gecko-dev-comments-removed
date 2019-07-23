










































#ifndef _PKIX_TARGETCERTCHECKER_H
#define _PKIX_TARGETCERTCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pkix_TargetCertCheckerState pkix_TargetCertCheckerState;

struct pkix_TargetCertCheckerState {
        PKIX_CertSelector *certSelector;
        PKIX_List *pathToNameList;
        PKIX_List *extKeyUsageList; 
        PKIX_List *subjAltNameList;
        PKIX_Boolean subjAltNameMatchAll;
        PKIX_UInt32 certsRemaining;
        PKIX_PL_OID *extKeyUsageOID;
        PKIX_PL_OID *subjAltNameOID;
};

PKIX_Error *
pkix_TargetCertChecker_Initialize(
        PKIX_CertSelector *certSelector,
        PKIX_UInt32 certsRemaining,
        PKIX_CertChainChecker **pChecker,
        void *plContext);

PKIX_Error *
pkix_TargetCertCheckerState_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
