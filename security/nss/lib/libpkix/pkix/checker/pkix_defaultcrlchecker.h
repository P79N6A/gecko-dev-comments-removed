










































#ifndef _PKIX_DEFAULTCRLCHECKER_H
#define _PKIX_DEFAULTCRLCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pkix_DefaultCRLCheckerState pkix_DefaultCRLCheckerState;

struct pkix_DefaultCRLCheckerState {
        PKIX_List *certStores; 
        PKIX_PL_Date *testDate;
        PKIX_Boolean certHasValidCrl;
        PKIX_Boolean nistCRLPolicyEnabled;
        PKIX_Boolean prevCertCrlSign;
        PKIX_PL_PublicKey *prevPublicKey; 
        PKIX_List *prevPublicKeyList; 
        PKIX_UInt32 reasonCodeMask;
        PKIX_UInt32 certsRemaining;
        PKIX_PL_OID *crlReasonCodeOID;

        PKIX_PL_X500Name *certIssuer;
        PKIX_PL_BigInt *certSerialNumber;
        PKIX_CRLSelector *crlSelector;
        PKIX_UInt32 crlStoreIndex;
        PKIX_UInt32 numCrlStores;
};

PKIX_Error *
pkix_DefaultCRLChecker_Initialize(
        PKIX_List *certStores,
        PKIX_PL_Date *testDate,
        PKIX_PL_PublicKey *trustedPubKey,
        PKIX_UInt32 certsRemaining,
        PKIX_Boolean nistCRLPolicyEnabled,
        PKIX_CertChainChecker **pChecker,
        void *plContext);

PKIX_Error *
pkix_DefaultCRLChecker_Check_Helper(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_PL_PublicKey *prevPublicKey,
        pkix_DefaultCRLCheckerState *state,
        PKIX_List *unresolvedCriticalExtensions,
        PKIX_Boolean useOnlyLocal,
        void **pNBIOContext,
        void *plContext);

PKIX_Error *
pkix_DefaultCRLChecker_Check_SetSelector(
        PKIX_PL_Cert *cert,
        pkix_DefaultCRLCheckerState *state,
        void *plContext);

PKIX_Error *
pkix_DefaultCRLCheckerState_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
