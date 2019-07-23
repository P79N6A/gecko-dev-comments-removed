










































#ifndef _PKIX_BUILD_H
#define _PKIX_BUILD_H
#include "pkix_tools.h"
#include "pkix_pl_ldapt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        BUILD_SHORTCUTPENDING,
        BUILD_INITIAL,
        BUILD_TRYAIA,
        BUILD_AIAPENDING,
        BUILD_COLLECTINGCERTS,
        BUILD_GATHERPENDING,
        BUILD_CERTVALIDATING,
        BUILD_ABANDONNODE,
        BUILD_CRLPREP,
        BUILD_CRL1,
        BUILD_DATEPREP,
        BUILD_CHECKTRUSTED,
        BUILD_CHECKTRUSTED2,
        BUILD_ADDTOCHAIN,
        BUILD_CHECKWITHANCHORS,
        BUILD_CRL2PREP,
        BUILD_CRL2,
        BUILD_VALCHAIN,
        BUILD_VALCHAIN2,
        BUILD_EXTENDCHAIN,
        BUILD_GETNEXTCERT
} BuildStatus;

typedef struct BuildConstantsStruct BuildConstants;







struct BuildConstantsStruct {
        PKIX_UInt32 numAnchors;
        PKIX_UInt32 numCertStores;
        PKIX_UInt32 numHintCerts;
        PKIX_UInt32 maxDepth;
        PKIX_UInt32 maxFanout;
        PKIX_UInt32 maxTime;
        PKIX_ProcessingParams *procParams;
        PKIX_PL_Date *testDate;
        PKIX_PL_Date *timeLimit;
        PKIX_PL_Cert *targetCert;
        PKIX_PL_PublicKey *targetPubKey;
        PKIX_List *certStores;
        PKIX_List *anchors;
        PKIX_List *userCheckers;
        PKIX_List *hintCerts;
        PKIX_CertChainChecker *crlChecker;
        PKIX_PL_AIAMgr *aiaMgr;
        PKIX_Boolean useAIAForCertFetching;
};

struct PKIX_ForwardBuilderStateStruct{
        BuildStatus status;
        PKIX_Int32 traversedCACerts;
        PKIX_UInt32 certStoreIndex;
        PKIX_UInt32 numCerts;
        PKIX_UInt32 numAias;
        PKIX_UInt32 certIndex;
        PKIX_UInt32 aiaIndex;
        PKIX_UInt32 anchorIndex;
        PKIX_UInt32 certCheckedIndex;
        PKIX_UInt32 checkerIndex;
        PKIX_UInt32 hintCertIndex;
        PKIX_UInt32 numFanout;
        PKIX_UInt32 numDepth;
        PKIX_UInt32 reasonCode;
        PKIX_Boolean dsaParamsNeeded;
        PKIX_Boolean revCheckDelayed;
        PKIX_Boolean canBeCached;
        PKIX_Boolean useOnlyLocal;
        PKIX_Boolean revChecking;
        PKIX_Boolean usingHintCerts;
        PKIX_Boolean certLoopingDetected;
        PKIX_PL_Date *validityDate;
        PKIX_PL_Cert *prevCert;
        PKIX_PL_Cert *candidateCert;
        PKIX_List *traversedSubjNames;
        PKIX_List *trustChain;
        PKIX_List *aia;
        PKIX_List *candidateCerts;
        PKIX_List *reversedCertChain;
        PKIX_List *checkedCritExtOIDs;
        PKIX_List *checkerChain;
        PKIX_List *revCheckers;
        PKIX_CertSelector *certSel;
        PKIX_VerifyNode *verifyNode;
        void *client; 
        PKIX_ForwardBuilderState *parentState;
        BuildConstants buildConstants;
};



PKIX_Error *
pkix_ForwardBuilderState_RegisterSelf(void *plContext);

PKIX_Error *
PKIX_Build_GetNBIOContext(void *state, void **pNBIOContext, void *plContext);

#ifdef __cplusplus
}
#endif

#endif
