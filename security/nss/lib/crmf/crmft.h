









#ifndef _CRMFT_H_
#define _CRMFT_H_


typedef enum {
    crmfVersion = 0,
    crmfSerialNumber = 1,
    crmfSigningAlg = 2,
    crmfIssuer = 3,
    crmfValidity = 4,
    crmfSubject = 5,
    crmfPublicKey = 6,
    crmfIssuerUID = 7,
    crmfSubjectUID = 8,
    crmfExtension = 9
} CRMFCertTemplateField;




typedef enum {
    crmfNoControl = 0,
    crmfRegTokenControl = 1,
    crmfAuthenticatorControl = 2,
    crmfPKIPublicationInfoControl = 3,
    crmfPKIArchiveOptionsControl = 4,
    crmfOldCertIDControl = 5,
    crmfProtocolEncrKeyControl = 6
} CRMFControlType;




typedef enum {
    crmfDontPublish = 0,
    crmfPleasePublish = 1
} CRMFPublicationAction;





typedef enum {
    crmfDontCare = 0,
    crmfX500 = 1,
    crmfWeb = 2,
    crmfLdap = 3
} CRMFPublicationMethod;




typedef enum {
    crmfNoArchiveOptions = 0,
    crmfEncryptedPrivateKey = 1,
    crmfKeyGenParameters = 2,
    crmfArchiveRemGenPrivKey = 3
} CRMFPKIArchiveOptionsType;




typedef enum {
    crmfNoPOPChoice = 0,
    crmfRAVerified = 1,
    crmfSignature = 2,
    crmfKeyEncipherment = 3,
    crmfKeyAgreement = 4
} CRMFPOPChoice;





typedef enum {
    crmfSender = 0,
    crmfPublicKeyMAC = 1
} CRMFPOPOSkiInputAuthChoice;




typedef enum {
    crmfNoSubseqMess = 0,
    crmfEncrCert = 1,
    crmfChallengeResp = 2
} CRMFSubseqMessOptions;




typedef enum {
    crmfNoMessage = 0,
    crmfThisMessage = 1,
    crmfSubsequentMessage = 2,
    crmfDHMAC = 3
} CRMFPOPOPrivKeyChoice;




typedef enum {
    crmfNoEncryptedKeyChoice = 0,
    crmfEncryptedValueChoice = 1,
    crmfEnvelopedDataChoice = 2
} CRMFEncryptedKeyChoice;


















typedef void (*CRMFEncoderOutputCallback) (void *arg,
					   const char *buf,
					   unsigned long len);





typedef SECItem* (*CRMFMACPasswordCallback) (void *arg);

typedef struct CRMFOptionalValidityStr      CRMFOptionalValidity;
typedef struct CRMFValidityCreationInfoStr  CRMFGetValidity;
typedef struct CRMFCertTemplateStr          CRMFCertTemplate;
typedef struct CRMFCertRequestStr           CRMFCertRequest;
typedef struct CRMFCertReqMsgStr            CRMFCertReqMsg;
typedef struct CRMFCertReqMessagesStr       CRMFCertReqMessages;
typedef struct CRMFProofOfPossessionStr     CRMFProofOfPossession;
typedef struct CRMFPOPOSigningKeyStr        CRMFPOPOSigningKey;
typedef struct CRMFPOPOSigningKeyInputStr   CRMFPOPOSigningKeyInput;
typedef struct CRMFPOPOPrivKeyStr           CRMFPOPOPrivKey;
typedef struct CRMFPKIPublicationInfoStr    CRMFPKIPublicationInfo;
typedef struct CRMFSinglePubInfoStr         CRMFSinglePubInfo;
typedef struct CRMFPKIArchiveOptionsStr     CRMFPKIArchiveOptions;
typedef struct CRMFEncryptedKeyStr          CRMFEncryptedKey;
typedef struct CRMFEncryptedValueStr        CRMFEncryptedValue;
typedef struct CRMFCertIDStr                CRMFCertID;
typedef struct CRMFCertIDStr                CRMFOldCertID;
typedef CERTSubjectPublicKeyInfo            CRMFProtocolEncrKey;
typedef struct CRMFValidityCreationInfoStr  CRMFValidityCreationInfo;
typedef struct CRMFCertExtCreationInfoStr   CRMFCertExtCreationInfo;
typedef struct CRMFPKMACValueStr            CRMFPKMACValue;
typedef struct CRMFAttributeStr             CRMFAttribute;
typedef struct CRMFControlStr               CRMFControl;
typedef CERTGeneralName                     CRMFGeneralName;
typedef struct CRMFCertExtensionStr         CRMFCertExtension;

struct CRMFValidityCreationInfoStr {
    PRTime *notBefore;
    PRTime *notAfter;
};

struct CRMFCertExtCreationInfoStr {
    CRMFCertExtension **extensions;
    int numExtensions;
};




extern const SEC_ASN1Template CRMFCertReqMessagesTemplate[];
extern const SEC_ASN1Template CRMFCertRequestTemplate[];


#endif 
