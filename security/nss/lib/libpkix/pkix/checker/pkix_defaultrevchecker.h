










































#ifndef _PKIX_DEFAULTREVCHECKER_H
#define _PKIX_DEFAULTREVCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_DefaultRevocationCheckerStruct {
        PKIX_CertChainChecker *certChainChecker;
        PKIX_CertChainChecker_CheckCallback check;
        PKIX_List *certStores;
        PKIX_PL_Date *testDate;
        PKIX_PL_PublicKey *trustedPubKey;
        PKIX_UInt32 certsRemaining;
};

PKIX_Error *
pkix_DefaultRevChecker_Initialize(
        PKIX_List *certStores,
        PKIX_PL_Date *testDate,
        PKIX_PL_PublicKey *trustedPubKey,
        PKIX_UInt32 certsRemaining,
        PKIX_RevocationChecker **pRevChecker,
        void *plContext);

PKIX_Error *
pkix_DefaultRevocationChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
