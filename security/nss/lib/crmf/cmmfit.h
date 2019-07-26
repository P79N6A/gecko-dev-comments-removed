




#ifndef _CMMFIT_H_
#define _CMMFIT_H_





























struct CMMFPKIStatusInfoStr {
    SECItem status;
    SECItem statusString;
    SECItem failInfo;
};

struct CMMFCertOrEncCertStr {
    union { 
        CERTCertificate    *certificate;
        CRMFEncryptedValue *encryptedCert;
    } cert;
    CMMFCertOrEncCertChoice choice;
    SECItem                 derValue;
};

struct CMMFCertifiedKeyPairStr {
    CMMFCertOrEncCert   certOrEncCert;
    CRMFEncryptedValue *privateKey;
    SECItem             derPublicationInfo; 





    SECItem unwrappedPrivKey;
};

struct CMMFCertResponseStr {
    SECItem               certReqId;
    CMMFPKIStatusInfo     status; 
    CMMFCertifiedKeyPair *certifiedKeyPair;
};

struct CMMFCertRepContentStr {
    CERTCertificate  **caPubs;
    CMMFCertResponse **response;
    PLArenaPool       *poolp;
    PRBool             isDecoded;
};

struct CMMFChallengeStr {
    SECAlgorithmID  *owf;
    SECItem          witness;
    SECItem          senderDER;
    SECItem          key;
    SECItem          challenge;
    SECItem          randomNumber;
};

struct CMMFRandStr {
    SECItem          integer;
    SECItem          senderHash;
    CERTGeneralName *sender;
};

struct CMMFPOPODecKeyChallContentStr {
    CMMFChallenge **challenges;
    PLArenaPool    *poolp;
    int             numChallenges;
    int             numAllocated;
};

struct CMMFPOPODecKeyRespContentStr {
    SECItem     **responses;
    PLArenaPool  *poolp;
};

struct CMMFKeyRecRepContentStr {
    CMMFPKIStatusInfo      status; 
    CERTCertificate       *newSigCert;
    CERTCertificate      **caCerts;
    CMMFCertifiedKeyPair **keyPairHist;
    PLArenaPool           *poolp;
    int                    numKeyPairs;
    int                    allocKeyPairs;
    PRBool                 isDecoded;
};

#endif 

