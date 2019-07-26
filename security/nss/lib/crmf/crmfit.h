





#ifndef _CRMFIT_H_
#define _CRMFIT_H_

struct CRMFCertReqMessagesStr {
    CRMFCertReqMsg **messages;
    PRArenaPool     *poolp;
};

struct CRMFCertExtensionStr {
    SECItem id;
    SECItem critical;
    SECItem value;
};


struct CRMFOptionalValidityStr {
    SECItem notBefore; 
    SECItem notAfter;
};

struct CRMFCertTemplateStr {
    SECItem                   version;
    SECItem                   serialNumber;
    SECAlgorithmID           *signingAlg;
    CERTName                 *issuer;
    CRMFOptionalValidity     *validity;
    CERTName                 *subject;
    CERTSubjectPublicKeyInfo *publicKey;
    SECItem                   issuerUID;
    SECItem                   subjectUID; 
    CRMFCertExtension       **extensions;
    int                       numExtensions;
};

struct CRMFCertIDStr {
    SECItem issuer; 
    SECItem serialNumber; 
};

struct CRMFEncryptedValueStr {
    SECAlgorithmID *intendedAlg;
    SECAlgorithmID *symmAlg;
    SECItem         encSymmKey; 
    SECAlgorithmID *keyAlg;
    SECItem         valueHint;  
    SECItem         encValue;   
};






struct CRMFEncryptedKeyStr {
    union {
        SEC_PKCS7ContentInfo   *envelopedData;
        CRMFEncryptedValue      encryptedValue; 
    } value;
    CRMFEncryptedKeyChoice encKeyChoice;
    SECItem derValue;
};


struct CRMFPKIArchiveOptionsStr {
    union {
        CRMFEncryptedKey  encryptedKey;
        SECItem           keyGenParameters;
        SECItem           archiveRemGenPrivKey; 
    } option;
    CRMFPKIArchiveOptionsType archOption;
};

struct CRMFPKIPublicationInfoStr {
    SECItem action; 
                    
    CRMFSinglePubInfo **pubInfos; 
};

struct CRMFControlStr {
    SECOidTag  tag;
    SECItem    derTag;
    SECItem    derValue;
    





    union {
        CRMFCertID              oldCertId;
        CRMFPKIArchiveOptions   archiveOptions;
        CRMFPKIPublicationInfo  pubInfo;
        CRMFProtocolEncrKey     protEncrKey; 
    } value;
};

struct CRMFCertRequestStr {
    SECItem            certReqId;
    CRMFCertTemplate   certTemplate;
    CRMFControl      **controls;
    


    PRArenaPool *poolp;
    PRUint32     requestID; 


};                                   

struct CRMFAttributeStr {
    SECItem derTag;
    SECItem derValue;
};

struct CRMFCertReqMsgStr {
    CRMFCertRequest            *certReq;
    CRMFProofOfPossession      *pop;
    CRMFAttribute             **regInfo;
    SECItem                     derPOP;
    

    PRArenaPool *poolp;
    PRBool       isDecoded;
};

struct CRMFPOPOSigningKeyInputStr {
    
    union {
        SECItem          sender; 
        CRMFPKMACValue  *publicKeyMAC;
    }authInfo;
    CERTSubjectPublicKeyInfo publicKey;
};

struct CRMFPOPOSigningKeyStr {
    SECItem                  derInput; 




    SECAlgorithmID          *algorithmIdentifier;
    SECItem                  signature; 
};                                      


struct CRMFPOPOPrivKeyStr {
    union {
        SECItem thisMessage; 
        SECItem subsequentMessage;  
        SECItem dhMAC; 
    } message;
    CRMFPOPOPrivKeyChoice messageChoice;
};


struct CRMFProofOfPossessionStr {
    union {
        SECItem             raVerified;
        CRMFPOPOSigningKey  signature;
        CRMFPOPOPrivKey     keyEncipherment;
        CRMFPOPOPrivKey     keyAgreement;
    } popChoice;
    CRMFPOPChoice       popUsed; 
};

struct CRMFPKMACValueStr {
    SECAlgorithmID algID;
    SECItem        value; 
};

struct CRMFSinglePubInfoStr {
    SECItem pubMethod; 





    CERTGeneralName *pubLocation; 
};

#endif 
