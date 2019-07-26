




#ifndef _CMMFT_H_
#define _CMMFT_H_

#include "secasn1.h"





typedef enum {
    cmmfNoCertOrEncCert = 0,
    cmmfCertificate = 1,
    cmmfEncryptedCert = 2
} CMMFCertOrEncCertChoice;





typedef enum {
    cmmfNoPKIStatus = -1,
    cmmfGranted = 0,
    cmmfGrantedWithMods = 1,
    cmmfRejection = 2,
    cmmfWaiting = 3,
    cmmfRevocationWarning = 4,
    cmmfRevocationNotification = 5,
    cmmfKeyUpdateWarning = 6,
    cmmfNumPKIStatus
} CMMFPKIStatus;





typedef enum {
    cmmfBadAlg = 0,
    cmmfBadMessageCheck = 1,
    cmmfBadRequest = 2,
    cmmfBadTime = 3,
    cmmfBadCertId = 4,
    cmmfBadDataFormat = 5,
    cmmfWrongAuthority = 6,
    cmmfIncorrectData = 7,
    cmmfMissingTimeStamp = 8,
    cmmfNoFailureInfo = 9
} CMMFPKIFailureInfo;

typedef struct CMMFPKIStatusInfoStr          CMMFPKIStatusInfo;
typedef struct CMMFCertOrEncCertStr          CMMFCertOrEncCert;
typedef struct CMMFCertifiedKeyPairStr       CMMFCertifiedKeyPair;
typedef struct CMMFCertResponseStr           CMMFCertResponse;
typedef struct CMMFCertResponseSeqStr        CMMFCertResponseSeq;
typedef struct CMMFPOPODecKeyChallContentStr CMMFPOPODecKeyChallContent;
typedef struct CMMFChallengeStr              CMMFChallenge;
typedef struct CMMFRandStr                   CMMFRand;
typedef struct CMMFPOPODecKeyRespContentStr  CMMFPOPODecKeyRespContent;
typedef struct CMMFKeyRecRepContentStr       CMMFKeyRecRepContent;
typedef struct CMMFCertRepContentStr         CMMFCertRepContent;





extern const SEC_ASN1Template CMMFCertRepContentTemplate[];
extern const SEC_ASN1Template CMMFPOPODecKeyChallContentTemplate[];

#endif 
